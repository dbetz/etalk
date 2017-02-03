Class foo
  IVars a,b;
  CVars last;
  
  CMethod [self new_a:aa b:bb]
    last := [[[super new] set_a:aa] set_b:bb];
  End CMethod
  
  CMethod [self last]
    last;
  End CMethod
  
  Method [self set_a:aa]
    a := aa;
    self;
  End Method
  
  Method [self set_b:bb]
    b := bb;
    self;
  End Method
  
  Method [self a]
    a;
  End Method
  
  Method [self b]
    b;
  End Method
End Class

Class bar : foo
  IVars c;
  
  CMethod [self new_a:aa b:bb c:cc]
    [[super new_a:aa b:bb] set_c:cc];
  End CMethod
  
  Method [self set_c:cc]
    c := cc;
    self;
  End Method
  
  Method [self c]
    c;
  End Method
End Class
 
Function main()
    cr := "
";
    last := 99;
    foo1 := [foo new_a:1 b:2];
    foo2 := [foo new_a:11 b:22];
    print("foo1=",foo1,cr);
    print("foo2=",foo2,cr);
    print("foo1:a=",[foo1 a],cr);
    print("foo2:a=",[foo2 a],cr);
    print("foo1:b=",[foo1 b],cr);
    print("foo2:b=",[foo2 b],cr);
    bar1 := [bar new_a:111 b:222 c:333];
    bar2 := [bar new_a:1111 b:2222 c:3333];
    print("bar1=",bar1,cr);
    print("bar2=",bar2,cr);
    print("bar1:a=",[bar1 a],cr);
    print("bar1:b=",[bar1 b],cr);
    print("bar1:c=",[bar1 c],cr);
    print("bar2:a=",[bar2 a],cr);
    print("bar2:b=",[bar2 b],cr);
    print("bar2:c=",[bar2 c],cr);
    print("last=",last,cr);
    print("[[foo last] a]=",[[foo last] a],cr);
    print("[[bar last] a]=",[[bar last] a],cr);
End Function

