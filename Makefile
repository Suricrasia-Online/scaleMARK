
# not using `pkg-config --libs` here because it will include too many libs
CFLAGS := `pkg-config --cflags gtk+-3.0` -lglib-2.0 -lGL -lgtk-3 -lgdk-3 -lgobject-2.0 -lspectre -no-pie -fno-plt -O1 -std=gnu11 -nostartfiles -Wall -Wextra -flto -fuse-linker-plugin -fno-unwind-tables -fno-asynchronous-unwind-tables -ffunction-sections -fdata-sections -fno-stack-check -fno-stack-protector -fomit-frame-pointer
PROJNAME := main

.PHONY: clean

all : $(PROJNAME)

packer : vondehi/vondehi.asm 
	cd vondehi; nasm -fbin -o vondehi vondehi.asm

shader.frag.min : shader.frag Makefile
	cp shader.frag shader.frag.min
	sed -i 's/m_origin/o/g' shader.frag.min
	sed -i 's/m_direction/d/g' shader.frag.min
	sed -i 's/m_point/k/g' shader.frag.min
	sed -i 's/m_color/i/g' shader.frag.min
	sed -i 's/m_attenuation/c/g' shader.frag.min
	sed -i 's/m_lag/m/g' shader.frag.min

	sed -i 's/MAXDEPTH/3/g' shader.frag.min
	sed -i 's/SAMPLES/8/g' shader.frag.min

	sed -i 's/\bRay\b/Co/g' shader.frag.min

shader.h : shader.frag.min Makefile
	mono ./shader_minifier.exe shader.frag.min -o shader.h

postscript.ps.min : postscript.ps
	cp postscript.ps postscript.ps.min
	sed -i '/^%[^%!]/d' postscript.ps.min

postscript.h : postscript.ps.min
	xxd -i $< > $@
	sed -i 's/unsigned char/const unsigned char/' $@

$(PROJNAME).elf : $(PROJNAME).c shader.h postscript.h Makefile
	gcc -o $@ $< $(CFLAGS)

$(PROJNAME)_unpacked : $(PROJNAME).elf
	mv $< $@

$(PROJNAME) : $(PROJNAME).elf.packed
	mv $< $@
	wc -c $(PROJNAME)

#all the rest of these rules just takes a compiled elf file and generates a packed version of it with vondehi
%_opt.elf : %.elf Makefile
	cp $< $@
	strip $@
	strip -R .note -R .comment -R .eh_frame -R .eh_frame_hdr -R .note.gnu.build-id -R .got -R .got.plt -R .gnu.version -R .rela.dyn -R .shstrtab $@
	#remove section header
	./Section-Header-Stripper/section-stripper.py $@

	#clear out useless bits
	sed -i 's/_edata/\x00\x00\x00\x00\x00\x00/g' $@;
	sed -i 's/__bss_start/\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00/g' $@;
	sed -i 's/_end/\x00\x00\x00\x00/g' $@;

	chmod +x $@

%.xz : % Makefile
	-rm $@
	lzma --format=lzma -9 --extreme --lzma1=preset=9,lc=0,lp=0,pb=0,nice=40,depth=32,dict=16384 --keep --stdout $< > $@

%.packed : %.xz packer Makefile
	cat ./vondehi/vondehi $< > $@
	chmod +x $@

clean :
	-rm *.elf *.xz shader.h $(PROJNAME) $(PROJNAME)_unpacked
