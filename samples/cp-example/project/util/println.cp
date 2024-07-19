namespace io;

const NEW_LINE: char = '\n';

fun println(value: any) {
  print(string(value) + NEW_LINE);
}
