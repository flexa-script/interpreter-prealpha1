// try-catch
using cp.core.exception;

try {
	print("executed try\n");
	var a = 10/0;
	print("after division");
} catch (var ex: cp::Exception) {
	print("executed catch\n");
	print("generated error: " + ex.error);
}
