Class Rectangle
  IVars x,y,height,width;

  CMethod [self new_x:xx y:yy height:hh width:ww]
    [[super new] initialize_x:xx y:yy height:hh width:ww];
  end CMethod

  Method [self initialize_x:xx y:yy height:hh width:ww]
    x := xx;
    y := yy;
    height := hh;
    width := ww;
    self;
  End Method

  Method [self show]
    MoveTo(x,y);
    LineTo(x+width,y);
    LineTo(x+width,y+height);
    LineTo(x,y+height);
    LineTo(x,y);
    self;
  End Method
End Class

Class Square : Rectangle

  CMethod [self new_x:xx y:yy size:ss]
    [[super new] initialize_x:xx y:yy size:ss];
  End CMethod

  Method [self initialize_x:xx y:yy size:ss]
    [super initialize_x:xx y:yy height:ss width:ss];
    dx := 0;
    dy := 0;
    self;
  End Method
End Class

Class MovingSquare : Square
  IVars dx,dy;
  CVars last;

  CMethod [self new_x:xx y:yy size:ss]
    [[super new] initialize_x:xx y:yy size:ss];
  End CMethod

  Method [self initialize_x:xx y:yy size:ss]
    [super initialize_x:xx y:yy height:ss width:ss];
  End Method

  Method [self speed_x:xx y:yy]
    dx := xx;
    dy := yy;
    self;
  End Method

  Method [self move]
	  x := x + dx;
	  y := y + dy;
	  [self show];
	  last := self;
	  self;
  End Method

  Method [self again]
    [last move];
  End Method
End Class

Function main()
  myrectangle := [Rectangle new_x:0 y:0 height:10 width:20];
  yourrectangle := [Rectangle new_x:10 y:20 height:20 width:40];
  mysquare := [Square new_x:10 y:50 size:10];
  yoursquare := [Square new_x:10 y:70 size:20];
  [myrectangle show];
  [yourrectangle show];
  [mysquare show];
  [yoursquare show];

  mover1 := [MovingSquare new_x:0 y:100 size:20];
  [mover1 speed_x:3 y:2];
  [mover1 show];

  mover2 := [MovingSquare new_x:0 y:150 size:10];
  [mover2 speed_x:4 y:4];
  [mover2 show];

  [mover1 move];
  [mover1 move];
  [mover1 again];
  [mover2 move];
  [mover2 move];
  [mover1 again];
End Function
