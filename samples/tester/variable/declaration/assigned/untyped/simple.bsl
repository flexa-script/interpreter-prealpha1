include namespace bsl;

start_test("DECLARATION / DEFINED / UNTYPED / SIMPLE");

var dbf = false;
assert_equals("VALUE false boolean as undeclared any", false, dbf);
assert_equals("TYPE false boolean as undeclared any", typeof(false), typeof(dbf));

var dbt = true;
assert_equals("VALUE true boolean as undeclared any", true, dbt);
assert_equals("TYPE true boolean as undeclared any", typeof(true), typeof(dbt));

var di = 666;
assert_equals("VALUE integer as undeclared any", 666, di);
assert_equals("TYPE integer as undeclared any", typeof(666), typeof(di));

var df = 6.66;
assert_equals("VALUE float as undeclared any", 6.66, df);
assert_equals("TYPE float as undeclared any", typeof(6.66), typeof(df));

var dc = 'c';
assert_equals("VALUE char as undeclared any", 'c', dc);
assert_equals("TYPE char as undeclared any", typeof('c'), typeof(dc));

var ds = "string";
assert_equals("VALUE string as undeclared any", "string", ds);
assert_equals("TYPE string as undeclared any", typeof("string"), typeof(ds));

end_test();
