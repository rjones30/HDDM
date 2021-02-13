all: guide.pdf

%.pdf: %.ps
	ps2pdf $^ $@

%.gz: %
	gzip $^

%.ps: %.dvi
	dvips -Ppdf -o $@ $^

%.dvi: %.tex
	latex $^
	latex $^

%.tar: %.tex $(wildcard %.eps)
	tar -zcf $@ $^

install: guide.pdf
	cp $^ ../users_guide.pdf

clean:
	rm -f *.aux *.log *.bak
