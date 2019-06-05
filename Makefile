
LIBS := -lSDL2 -lGL -lspectre -lopus
CFLAGS := -fno-plt -Os -std=gnu11 -nostartfiles -Wall -Wextra
CFLAGS += -fno-stack-protector -fno-stack-check -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer
CFLAGS += -no-pie -fno-pic -fno-PIE -fno-PIC -march=core2 -ffunction-sections -fdata-sections
PROJNAME := scalemark

CANVAS_WIDTH_SMALL := 640
CANVAS_HEIGHT_SMALL := 360

CANVAS_WIDTH_FULL := 1920
CANVAS_HEIGHT_FULL := 1080

.PHONY: clean checkgccversion

all : checkgccversion $(PROJNAME) $(PROJNAME)_small $(PROJNAME).zip

checkgccversion :
ifneq ($(shell expr `gcc -dumpversion`),8.3.0)
	$(error GCC version must be 8.3.0 (if newer, just remove this check. You can find it, I beleive in you. Just look at the makefile and use ctrl+f. The best piece of advice I've ever gotten for fixing issues with code I do not own is "follow the strings." If you do that, everything will come to you))
endif

$(PROJNAME).zip : $(PROJNAME) $(PROJNAME)_small $(PROJNAME)_unpacked $(PROJNAME)_small_unpacked README.txt screenshot.png
	zip $@ $^

packer : vondehi/vondehi.asm 
	cd vondehi; nasm -fbin -o vondehi vondehi.asm

.PHONY: noelfver

noelfver:
	make -C noelfver

shader.frag.min : shader.frag Makefile
	cp $< $@
	sed -i 's/m_origin/o/g' $@
	sed -i 's/m_direction/d/g' $@
	sed -i 's/m_point/k/g' $@
	sed -i 's/m_color/i/g' $@
	sed -i 's/m_attenuation/c/g' $@
	sed -i 's/m_lag/m/g' $@

	sed -i 's/MAXDEPTH/3/g' $@

	sed -i 's/\bRay\b/Co/g' $@

shader_small.frag.min : shader.frag.min
	cp $< $@
	sed -i 's/SAMPLES/1/g' $@
	sed -i 's/CANVAS_WIDTH/$(CANVAS_WIDTH_SMALL)/g' $@
	sed -i 's/CANVAS_HEIGHT/$(CANVAS_HEIGHT_SMALL)/g' $@

shader_full.frag.min : shader.frag.min
	cp $< $@
	sed -i 's/SAMPLES/2/g' $@
	sed -i 's/CANVAS_WIDTH/$(CANVAS_WIDTH_FULL)/g' $@
	sed -i 's/CANVAS_HEIGHT/$(CANVAS_HEIGHT_FULL)/g' $@

shader_small.h : shader_small.frag.min Makefile
	mono ./shader_minifier.exe $< -o $@

shader_full.h : shader_full.frag.min Makefile
	mono ./shader_minifier.exe $< -o $@

postscript.ps.min : postscript.ps Makefile
	cp $< $@
	sed -i '/^%[^%!]/d' $@
	sed -i '/^$$/d' $@
	sed -i 's/\(BoundingBox: 0 0\) 1024 2048/\1 4096 4096\n4.0 2.0 scale/' $@
	sed -i -z 's/\(%[^\n]\+\)/\1######/g;s/\n\(.\)/ \1/g;s/###### /\n/g' $@


postscript.h : postscript.ps.min
	xxd -i $< > $@
	sed -i 's/unsigned char/const unsigned char/' $@

$(PROJNAME).elf : $(PROJNAME).c shader_full.h postscript.h Makefile sys.h def.h
	gcc -o $@ $< $(CFLAGS) $(LIBS) -DCANVAS_WIDTH=$(CANVAS_WIDTH_FULL) -DCANVAS_HEIGHT=$(CANVAS_HEIGHT_FULL) -DFULLSIZE

$(PROJNAME)_small.elf : $(PROJNAME).c shader_small.h postscript.h Makefile sys.h def.h
	gcc -o $@ $< $(CFLAGS) $(LIBS) -DCANVAS_WIDTH=$(CANVAS_WIDTH_SMALL) -DCANVAS_HEIGHT=$(CANVAS_HEIGHT_SMALL)

$(PROJNAME)_unpacked : $(PROJNAME).elf
	mv $< $@

$(PROJNAME)_small_unpacked : $(PROJNAME)_small.elf
	mv $< $@

$(PROJNAME) : $(PROJNAME)_opt.elf.packed
	mv $< $@
	wc -c $@

$(PROJNAME)_small : $(PROJNAME)_small_opt.elf.packed
	mv $< $@
	wc -c $@

#all the rest of these rules just takes a compiled elf file and generates a packed version of it with vondehi
%_opt.elf : %.elf Makefile noelfver
	cp $< $@
	strip $@
	strip -R .note -R .comment -R .eh_frame -R .eh_frame_hdr -R .note.gnu.build-id -R .got -R .got.plt -R .gnu.version -R .shstrtab -R .gnu.version_r -R .gnu.hash $@
	./noelfver/noelfver $@ > $@.nover
	mv $@.nover $@
	#remove section header
	./Section-Header-Stripper/section-stripper.py $@

	#clear out useless bits
	sed -i 's/_edata/\x00\x00\x00\x00\x00\x00/g' $@;
	sed -i 's/__bss_start/\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/g' $@;
	sed -i 's/_end/\x00\x00\x00\x00/g' $@;
	sed -i 's/GLIBC_2\.2\.5/\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/g' $@;

	chmod +x $@

%.xz : % Makefile
	-rm $@
	lzma --format=lzma -9 --extreme --lzma1=preset=9,lc=0,lp=0,pb=0,nice=110,depth=32,dict=16384 --keep --stdout $< > $@

%.packed : %.xz packer Makefile
	cat ./vondehi/vondehi $< > $@
	chmod +x $@

clean :
	-rm *.elf *.xz shader.h $(PROJNAME) $(PROJNAME)_unpacked $(PROJNAME)_small $(PROJNAME)_small_unpacked
