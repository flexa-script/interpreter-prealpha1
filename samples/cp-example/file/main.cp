using cp.core.exception;

try{
  //var i: int = "asda";
  var i = 10 / 0;
  print(i);
}catch(var ex: cp::Exception){
  print(ex.error);
}

// var i = 10 / 0;
// print(i);
