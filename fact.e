Function factorial(n)
  If n = 0 Then
    1;
  Else
    n * factorial(n - 1);
  End If
End Function

Function main()
  print(factorial(10),"
");
End Function
