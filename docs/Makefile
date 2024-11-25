DOC=doc

$(DOC).pdf: $(DOC).tex
	pdflatex $(DOC).tex
	bibtex $(DOC).aux
	pdflatex $(DOC).tex
	pdflatex $(DOC).tex

clean:
	rm -f $(DOC).aux $(DOC).dvi $(DOC).log $(DOC).ps $(DOC).out $(DOC).bbl $(DOC).blg

clean-all: clean
	rm -f $(DOC).pdf
