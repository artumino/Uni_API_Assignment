shopt -s nullglob
for f in ./tests/*.in
do
	./main < $f > $f.out
	#mv $f.out ./taken
done
