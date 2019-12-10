# bitPrec
Analyzing real valued expressions for their bit precision requirement in different refinement levels for the julia-tsa-core and juliatsa3d-core projects.

### Disclaimer

Although I tried my best to ensure correctness of the implementation and therefore the results,
the code comes with no warranty.

## README is organized as:

1. Quick start
2. Background and limitations
3. Command-line parameters
4. Explaining the output file
5. Table of built-in functions
6. Contact and links


## (1) Quick start

Analyzing a real-valued expression. Results are written in file `_zertifikat.user_bit.precision_R4.txt`.
<br>`bitPrec.exe "func=x* x-y *y+d" range=4 d=2,20``

Analyzing a complex built-in function, real and imaginary part separately. Results in respective text file.
<br>`bitPrec.exe func=z3azc range=8`

Analyzing a triplex built-in function, x,y,z coordinates separately. Results in respective text file.
<br>`bitPrec.exe func=tricz4b range=8`

## (1) Background and limitations

The bit precision analysis is performed using an expanded form (bracket-free), analyzing every term in its absolute value (determines how
many bits for the integer part are necessary) and the fractional bits of every term. 

The sum of all absolute maximum term values determines the integer part of the final expression, the maximum fractional bits for any term
determine the bits for the fractional part of the final result.

The software's parser is very simple and restricted: 

- Supported operations are binary +-*^, but no unary minus.
- Expressions must be bracket-free and in expanded form.
- Variables are only one-letter characters a to z, where x,y,z are dependent on the refinement level and hence also their fractional bits, for the other variables their limits must be provided in user-entered formulas (see command-line options).
- Numbers must be positive and one-digit. Expressions like 12 *x must be entered as e.g. 5 *x+7 *x.
- Complex or triplex functions must be analyzed by their respective component functions.


## (2) Command-line parameters (case-insensitive, order not relevant)

`FUNC=string` (if not provided, standard value is Z2C)
<br>User entered expression or the name of an implemented function (see table 4).

`RANGE=integer` (if not provided, values 2, 4 and 8 are used in a loop)
<br>x,y,z axis offer -range .. +range. This determines the fractional bits for the variables x,y,z which increase in higher refinement levels.

e.g. 'D=4,19' 
<br>or in general `variable symbol=integer,integer`
<br>For user-entered expressions, the maximum absolute value for a variable and their fractional bits must be provided.
The above example means |d|<=4 with at most 19 fractional bits.

## (4) Explainging the output

An example output for the expression:

	x^2-y*y+d
	|d|<=3, max fracBits=19
	|x|<=4, max fracBits=25
	|y|<=4, max fracBits=25

	axisrange = --4 .. +4
	SCALE := (2*axis range) / (2^REFINEMENT LEVEL == pixels per axis)
	  := 2^-(REF-3)

                        maximum absolute value of a term
		                                bits needed for the maximum absolute value
  		                                    fractional bits of the term
    |f_new| <=
              |x^2|	      ||maxw= 16	5.0	  2*(.(SCALE))
              + |y* y|    ||maxw= 16	5.0	  (.(SCALE))+(.(SCALE))
              + |d|	      ||maxw=  3	2.0	  .19
              ------------------------------------------
                          ||maxw=  35	6.0	  max(2*(.(SCALE)),(.(SCALE))+(.(SCALE)),.19)
    
                      REF 10		6.19 = 25	=> double

The SCALE determines the fractional bits for x,y,z coordinates in different refinement levels.
<br>||maxw denotes the maximum absolute value of a given term.
<br>`5.0` e.g. denotes the integer part's bit requirement (5 in front of the radix point, 0 behind)
<br>`2*(.(SCALE))`denotes the fractional bits, for x^2  the term needs twice the bits of the variable x.
<br>REF 10 ... => double: at refinement level with the given axis range, the datatype double is sufficient.




## (5) Built-in functions

All variables except x,y,z have maximum absolute value of 2 and fractional bits of 25.
<br>Expanded forms were computed thanks to WolframAlpha.

<table>
<tr><td>name</td><td>component expressions</td></tr>
<tr><td>z2c</td><td>
x_new := x^2-y^2 + d
<br>y_new := 2*x*y + e
</td></tr>
<tr><td>z3azc</td><td>
x_new := d + f*x-g*y+x^3-3*x*y^2
<br>y_new := e + f*y+g*x+3*x^2*y-y^3"
</td></tr>
<tr><td>z4azc</td><td>
x_new := d + f*x-g*y+x^4-6*x^2*y^2+y^4
<br>y_new := e + f*y+g*x+4*x^3*y-4*x*y^3
</td></tr>
<tr><td>z5azc</td><td>
x_new := d + f*x-g*y+x^5-2*5*x^3*y^2+5*x*y^4
<br>y_new := e + f*y+g*x+5*x^4*y-2*5*x^2*y^3+y^5
</td></tr>
<tr><td>z6azc</td><td>
x_new := d+f*x-g*y+x^6-3*5*x^4*y^2+3*5*x^2*y^4-y^6
<br>y_new := f*y+g*x+6*x^5*y-4*5*x^3*y^3+6*x*y^5+e
</td></tr>
<tr><td>TRICZ4B</td><td>
x_new := r + f*x+x^4-6*x^2*y^2+y^4-h*z^2
<br>y_new := s + f*y+4*x^3*y-4*y^3*x
<br>z_new := t + z^3-y^3-x^3
</td></tr>
</table>


## (6) Contact

Please direct any comments to:

marcm200@freenet.de

forum: https://fractalforums.org/fractal-mathematics-and-new-theories/28/julia-sets-true-shape-and-escape-time/2725

Marc Meidlinger, December 2019

