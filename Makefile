
# not using `pkg-config --libs` here because it will include too many libs
LIBS := -lSDL2 -lGL -lspectre -lopus
CFLAGS := -fno-plt -Os -std=gnu11 -nostartfiles -Wall -Wextra
CFLAGS += -fno-stack-protector -fno-stack-check -fno-unwind-tables -fno-asynchronous-unwind-tables -fomit-frame-pointer
CFLAGS += -no-pie -fno-pic -fno-PIE -fno-PIC -march=core2 -ffunction-sections -fdata-sections
PROJNAME := main

.PHONY: clean

all : $(PROJNAME)

packer : vondehi/vondehi.asm 
	cd vondehi; nasm -fbin -o vondehi vondehi.asm

.PHONY: noelfver

noelfver:
	make -C noelfver

shader.frag.min : shader.frag Makefile
	cp shader.frag shader.frag.min
	sed -i 's/m_origin/o/g' shader.frag.min
	sed -i 's/m_direction/d/g' shader.frag.min
	sed -i 's/m_point/k/g' shader.frag.min
	sed -i 's/m_color/i/g' shader.frag.min
	sed -i 's/m_attenuation/c/g' shader.frag.min
	sed -i 's/m_lag/m/g' shader.frag.min

	sed -i 's/MAXDEPTH/3/g' shader.frag.min
	sed -i 's/SAMPLES/1/g' shader.frag.min

	sed -i 's/\bRay\b/Co/g' shader.frag.min

shader.h : shader.frag.min Makefile
	mono ./shader_minifier.exe shader.frag.min -o shader.h

postscript.ps.min : postscript.ps Makefile
	cp postscript.ps postscript.ps.min
	sed -i '/^%[^%!]/d' postscript.ps.min
	sed -i 's/\(%[^\n]\+\)/\1######/g;s/\n\(.\)/ \1/g;s/###### /\n/g' postscript.ps.min

postscript.h : postscript.ps.min
	xxd -i $< > $@
	sed -i 's/unsigned char/const unsigned char/' $@

$(PROJNAME).elf : $(PROJNAME).c shader.h postscript.h Makefile sys.h def.h
	gcc -o $@ $< $(CFLAGS) $(LIBS)

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
	lzma --format=lzma -9 --extreme --lzma1=preset=9,lc=0,lp=0,pb=0,nice=40,depth=32,dict=16384 --keep --stdout $< > $@

%.packed : %.xz packer Makefile
	cat ./vondehi/vondehi $< > $@
	chmod +x $@

.PRECIOUS: $(PROJNAME)_opt.elf

clean :
	-rm *.elf *.xz shader.h $(PROJNAME) $(PROJNAME)_unpacked
