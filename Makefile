
LIBS := -lSDL2 -lGL -lspectre -lopus
CFLAGS := -fno-plt -Os -std=gnu11 -nostartfiles -Wall -Wextra
CFLAGS += -fno-stack-protector -fno-stack-check -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer
CFLAGS += -no-pie -fno-pic -fno-PIE -fno-PIC -march=core2 -ffunction-sections -fdata-sections
PROJNAME := main

CANVAS_WIDTH := 480
CANVAS_HEIGHT := 270

.PHONY: clean

all : $(PROJNAME)

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
	sed -i 's/SAMPLES/4/g' $@
	sed -i 's/CANVAS_WIDTH/$(CANVAS_WIDTH)/g' $@
	sed -i 's/CANVAS_HEIGHT/$(CANVAS_HEIGHT)/g' $@

	sed -i 's/\bRay\b/Co/g' $@

shader.h : shader.frag.min Makefile
	mono ./shader_minifier.exe $< -o $@

postscript.ps.min : postscript.ps Makefile
	cp $< $@
	sed -i '/^%[^%!]/d' $@
	sed -i '/^$$/d' $@
	sed -i 's/\(BoundingBox: 0 0 1024\) 2048/\1 1024\n1.0 0.5 scale/' $@
	sed -i -z 's/\(%[^\n]\+\)/\1######/g;s/\n\(.\)/ \1/g;s/###### /\n/g' $@


postscript.h : postscript.ps.min
	xxd -i $< > $@
	sed -i 's/unsigned char/const unsigned char/' $@

$(PROJNAME).elf : $(PROJNAME).c shader.h postscript.h Makefile sys.h def.h
	gcc -o $@ $< $(CFLAGS) $(LIBS) -DCANVAS_WIDTH=$(CANVAS_WIDTH) -DCANVAS_HEIGHT=$(CANVAS_HEIGHT)

$(PROJNAME)_unpacked : $(PROJNAME).elf
	mv $< $@

$(PROJNAME) : $(PROJNAME)_opt.elf.packed
	mv $< $@
	wc -c $(PROJNAME)

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
	lzma --format=lzma -9 --extreme --lzma1=preset=9,lc=0,lp=0,pb=0,nice=80,depth=32,dict=16384 --keep --stdout $< > $@

%.packed : %.xz packer Makefile
	cat ./vondehi/vondehi $< > $@
	chmod +x $@

.PRECIOUS: $(PROJNAME)_opt.elf

clean :
	-rm *.elf *.xz shader.h $(PROJNAME) $(PROJNAME)_unpacked
