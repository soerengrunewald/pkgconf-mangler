use std::env;
use std::fs;
use std::io;
//use std::io::{self, Write};
use std::path::Path;

fn merge_private_lines(lines: &mut Vec<String>)
{
    let mut private_map: std::collections::HashMap<String, String> = std::collections::HashMap::new();
    let mut merged_lines = Vec::new();

    for line in lines.iter() {
        if line.ends_with(".private") {
            // Extract the base line
            let base_line = &line[..line.len() - ".private".len()]; // remove ".private"
            private_map.insert(base_line.to_string(), line.clone());
        } else {
            if private_map.contains_key(line) {
                // If a non-private line matches a private line, merge them
                let private_line_var = private_map.remove(line).unwrap();
                merged_lines.push(format!("{} {}", private_line_var, line));
            } else {
                merged_lines.push(line.clone());
            }
        }
    }

    *lines = merged_lines;
}


fn main() -> io::Result<()>
{
    // Parsing command line arguments
    let args: Vec<String> = env::args().collect();
    if args.len() < 2 {
        eprintln!("Usage: {} <path-to-pkg-config-file> [--merge] [--remove-rpath]", args[0]);
        return Ok(());
    }

    let pkgconf_file_name = &args[1];
    let merge = args.iter().any(|arg| arg == "--merge");
    let remove_rpath = args.iter().any(|arg| arg == "--remove-rpath");

    // Read the contents of the pkg-config file
    let contents = fs::read_to_string(pkgconf_file_name)?;
    let mut lines: Vec<String> = contents.lines().map(|line| line.to_string()).collect();

    // Merge .private lines with non-private lines if --merge flag is provided
    if merge {
        merge_private_lines(&mut lines);
    }

    // Remove rpath entries if --remove-rpath flag is provided
    if remove_rpath {
        lines.retain(|line| !line.contains("rpath:"));
    }

    // Output the modified contents
    let output = lines.join("\n");
    println!("{}", output);

    //Optionally: save changes to the same file or a new file
    let output_path = Path::new(pkgconf_file_name).with_extension("modified.pc");
    fs::write(output_path, output)?;

    Ok(())
}

