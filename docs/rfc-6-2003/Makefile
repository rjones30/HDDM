all: draft-1_1.pdf

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

clean:
	rm -f *.aux *.log
