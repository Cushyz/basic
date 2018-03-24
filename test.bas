0	PRINT "HELLO"
10	FOR I=0 TO 10
20	 PRINT "I is:";I
30	NEXT
40	PRINT "How many '*' do you want"
50	INPUT N
60	PRINT "*";
70	N = N-1
80	IF N<>0 THEN GOTO 60
90	PRINT ""
100 GOSUB 500
110 PRINT "A random number:";RANDOM
120	END 'End of a program
500	PRINT "This is a sub"
510	RETURN 