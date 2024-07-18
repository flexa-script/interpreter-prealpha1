var i = 10;

// fun ai(x: float){
// 	x+=x;
// }

fun ai(x: int){
	x+=x;
}

ai(i);
println(i);

ai(unref i);
println(i);

ai(ref i);
println(i);

ai(unref i);
println(i);

ai(ref i);
println(i);
