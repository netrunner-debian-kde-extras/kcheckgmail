#!/bin/sh

for e in 16 22 32 48 64 128 ; do
	for r in kcheckgmail kcheckgmaillight kcheckgmailauth ; do
		rm -f ../cr$e-app-$r.png
		inkscape -z -f $r.svg -e ../cr$e-app-$r.png -b white -w $e -h $e -y 0.0
	done
done
