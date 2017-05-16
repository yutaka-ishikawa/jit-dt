MAIN = jit-dt
FIGS = $(patsubst %.png,%.xbb, $(wildcard *.png))
.SUFFIXES: .png .xbb
.png.xbb:
	ebb -x $<

all: $(MAIN).tex $(FIGS)
	platex -kanji=euc $(MAIN).tex
	platex -kanji=euc $(MAIN).tex
	dvipdfmx -p a4 -o $(MAIN).pdf $(MAIN).dvi
