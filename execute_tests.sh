shopt -s nullglob
for f in ./tests/*.in
do
	main < $f > ${f##*/}.out
done
