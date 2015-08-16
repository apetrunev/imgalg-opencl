#!/bin/sh

#
# ./gaussian.sh -n N
#

OCTAVE=$(which octave)
PROG=$(basename $0)
TMP=$(getopt -o n: -n "$PROG" -- "$@")
N=

[ $? != 0 ] && exit 1;

eval set -- "$TMP"

while true; do
	case "$1" in
	-n)
		N=$(echo "$2" | sed -E -e 's/^[[:space:]]+//g' -e 's/[[:space:]]+$//g')
		shift 2;;
	--)
		shift
		break;;
	*)
		echo "error: unknown option"
		exit 1;;
	esac
done

# verify that N is a number
[ -n "$N" ] && echo "$N" | grep -E "^[[:digit:]]+$" >/dev/null || exit 1
# set default value if no N is specified
[ -z "$N" ] && N=3

$OCTAVE --silent --eval "
function retval = gauss(i,j,s)
	coeff = 1/(s*sqrt(2*pi));
	pwr = -(i*i + j*j)/2*s*s;
	retval = coeff*exp(pwr);
endfunction

if (rem($N,2) == 0)
	N = $N + 1;
else
	N = $N;
endif

x = -fix(N/2):1:fix(N/2);
y = -fix(N/2):1:fix(N/2);

kernel = zeros(N,N);

for i = 1:N
	for j = 1:N
		kernel(i,j) = gauss(x(i),y(j),1);	
	endfor
endfor

# find normalization coefficient
c = 1.0 / kernel(N,N);

normalized = round(c*kernel);

printf('gauss%d %s\n', N, mat2str(normalized, 3)); 
printf('gauss_sum %d\n', sum(normalized(:)));
printf('gauss_dim %d\n', N);

" | gawk '
match($0, /^gauss[[:digit:]]+[[:space:]]+[[](.*)+[]]$/, arr) {
	split(arr[1], rows, ";")
}

match($0, /^gauss_sum[[:space:]]+([[:digit:]]+)$/, arr) {
	sum = arr[1]
}

match($0, /gauss_dim[[:space:]]+([[:digit:]]+)$/, arr) {
	dim = arr[1]
}

END {
	printf("#ifndef GAUSS_H_\n")
	printf("#define GAUSS_H_\n")
	printf("\nint gauss[] = {\n")
	
	for (i = 1; i <= length(rows); i++) {
		gsub(/[[:space:]]/, ",", rows[i])
		comma = i == length(rows) ? "": ","
		printf("\t%s%s\n", rows[i], comma)
	}

	printf("};\n\n")
	printf("int gauss_sum = %d;\n", sum)
	printf("int gauss_dim = %d;\n", dim)
	printf("\n#endif /* GAUSS_H_ */\n")
}
' 
