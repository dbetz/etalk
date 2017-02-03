Function test(a,b)
  a + if b < 0 then 1 else 2

Function factorial(n)
  If n = 0
    Then 1
    Else n * factorial(n - 1)

Function square(x,y,size)
  Begin
    MoveTo(x,y);
    LineTo(x+size,y);
    LineTo(x+size,y+size);
    LineTo(x,y+size);
    LineTo(x,y);
  End

Function main()
  Begin
    factorial(10);
    square(0,0,100);
    square(5,5,100);
    square(10,10,100);
  End
