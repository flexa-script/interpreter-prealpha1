using cp.core.HTTP;

as namespace cp;

var config = HttpConfig {
	hostname="example.com",
	method=GET
};

var res = request(config);

println("Response:");
println("--------------------------------------------");
println("http_version: ", res.http_version);
println("--------------------------------------------");
println("status: ", res.status);
println("--------------------------------------------");
println("status_description: ", res.status_description);
println("--------------------------------------------");
var headers = to_array(res.headers);
println("Headers:");
foreach (var res in headers) {
	println(res.key, ": ", res.value);
}
println("--------------------------------------------");
println("data:\n", res.data, "\n");
println("--------------------------------------------");
println("raw:\n", res.raw);
println("--------------------------------------------");
