PROJ=dokumentace

.PHONY: all clean

all: $(PROJ).pdf

$(PROJ).pdf: $(PROJ).tex Literatura/literatura.bib Literatura/czplain.bst
	pdflatex $(PROJ).tex
	bibtex $(PROJ)
	pdflatex $(PROJ).tex
	pdflatex $(PROJ).tex
	$(MAKE) clean

clean:
	rm -f *.aux *.dvi *.log *.bbl *.blg *.ps *.out *.zip *.bi~ *.te~ *.toc
