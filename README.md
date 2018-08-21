# basic
A simple basic interpreter.
## Usage
>./basic < file name >

File name should end with '.bas'.

## Grammar
	line ::= [number] statement [note] CR
	statement ::= "PRINT" print-list
				| "INPUT" var
				| "IF" expression "THEN" statement
				| "FOR" var "=" expression "TO" expression [ "STEP" expression ] "THEN" statement
				| "NEXT"
				| var = expression
				| "GOTO" number
				| "GOSUB" number
				| "RETURN"
				| "END"
	print-list	::= (string | expression) { (","|";") (string | expression) }
	expression	::= addsub { (">"|"<"|"<="|">="|"<>") addsub }
	addsub		::= term { ("+"|"-") term }
	term		::= pow { ("*"|"/"|"%") pow }
	pow 		::= factor { "^" factor }
	factor		::= ["+"|"-"] ( var | number |"("expression")" )
-----------
	var		= /[A-Z]/
	number	= /[0-9]*(.[0-9]+)?e[0-9]+/
	string	= /"[^"]+"/