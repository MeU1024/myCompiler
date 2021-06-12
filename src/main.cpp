#include <iostream>
#include "parser.tab.hpp"
#include "basis.hpp"
#include "Vis.hpp"

extern FILE *yyin;
std::shared_ptr<fpc::ProgramNode> program;

int main(int argc, char *argv[])
{
    if (argc < 2) return 1;
    yyin = fopen(argv[1], "r");
    parser p;
    p.parse();
    fpc::ASTvis vis;
    vis.travAST(program);
    return 0;
}
