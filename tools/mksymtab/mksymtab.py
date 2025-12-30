import sys

with open(sys.argv[1], "r") as srcfile, open(sys.argv[2], "w") as destfile:
    lines = srcfile.readlines()
    lines = [line[:-1] if line[-1] == '\n' else line for line in lines]
    
    destfile.write("#include \"module/symbol.h\"\n\n")
    destfile.write("#include <stddef.h>\n\n")
    
    for line in lines:
        destfile.write(f"extern int {line};\n")
        
    destfile.write("\n\nconst struct symbol _exported_symbols[] = {\n")
    for line in lines:
        destfile.write(f"    {{ \"{line}\", &{line} }},\n")
    destfile.write("    { NULL, NULL },\n};\n")
    