start_test("DECLARATION / UNDEFINED / ARRAY");

print("declaring 'uax' as undeclared any");
var ua1[3];
var ua2[3][3];
var ua3[3][3][3];
println(" ... done");

print("declaring 'uaax' as any");
var uaa1[3]: any;
var uaa2[3][3]: any;
var uaa3[3][3][3]: any;
println(" ... done");

print("declaring 'uabx' as bool");
var uab1[3]: bool;
var uab2[3][3]: bool;
var uab3[3][3][3]: bool;
println(" ... done");

print("declaring 'uaix' as int");
var uai1[3]: int;
var uai2[3][3]: int;
var uai3[3][3][3]: int;
println(" ... done");

print("declaring 'uafx' as float");
var uaf1[3]: float;
var uaf2[3][3]: float;
var uaf3[3][3][3]: float;
println(" ... done");

print("declaring 'uacx' as char");
var uac1[3]: char;
var uac2[3][3]: char;
var uac3[3][3][3]: char;
println(" ... done");

print("declaring 'uasx' as string");
var uas1[3]: string;
var uas2[3][3]: string;
var uas3[3][3][3]: string;
println(" ... done");

end_test();
