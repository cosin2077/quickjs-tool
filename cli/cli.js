import * as std from 'std';
import * as os from 'os';
import { fetch } from './http.so';

function readFile(path) {
  let file = std.open(path, 'r');
  if (!file) {
    std.err.printf("Error: Cannot open file %s\n", path);
    std.exit(1);
  }
  let content = file.readAsString();
  file.close();
  return content;
}

function writeFile(path, content) {
  let file = std.open(path, 'w');
  if (!file) {
    std.err.printf("Error: Cannot open file %s\n", path);
    std.exit(1);
  }
  file.puts(content);
  file.close();
}

let args = scriptArgs.slice(1);

if (args.length === 0 || args[0] === '--help') {
  std.out.puts(`
Usage: mycli <command> [args]
Commands:
  read <file>           Read file content
  write <file> <content> Write content to file
  http <url>            Send HTTP GET request
  exec <bin> [args]     Execute binary with arguments
`);
  std.exit(0);
}

if (args[0] === 'read' && args.length === 2) {
  std.out.puts(readFile(args[1]));
} else if (args[0] === 'write' && args.length === 3) {
  writeFile(args[1], args[2]);
  std.out.puts("Write successful\n");
} else if (args[0] === 'http' && args.length === 2) {
  let response = fetch(args[1]);
  std.out.puts(response);
} else if (args[0] === 'exec' && args.length >= 2) {
  let ret = os.exec(args.slice(1));
  if (ret !== 0) {
    std.err.printf("Exec failed with code %d\n", ret);
    std.exit(ret);
  }
} else {
  std.err.puts("Error: Unknown command or invalid arguments. Use --help for usage.\n");
  std.exit(1);
}