\ENCODING=ISO-8859-1
\Problem name: patGenerator

Minimize
 obj: x1 - 0.25 a_0 - 0.25 a_1 - 0.5 a_2 - 0.5 a_3 - 0.75 a_4
Subject To
 conWidth: 25 a_0 + 40 a_1 + 50 a_2 + 55 a_3 + 70 a_4 <= 115
Bounds
      x1 = 1
      a_0 >= 0
      a_1 >= 0
      a_2 >= 0
      a_3 >= 0
      a_4 >= 0
Generals
 a_0  a_1  a_2  a_3  a_4 
End
