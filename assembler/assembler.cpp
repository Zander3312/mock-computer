#ifndef __ASSEMBLER_CPP__
#define __ASSEMBLER_CPP__

#include "assembler.h"
#include <vector>
#include <string>
#include <map>
#include <iostream>
#include <sstream>
#include <fstream>

int main(int argc, char* argv[]) {
    if (argc < 4) // Checks that at least 3 arguments are given in command line
    {
        std::cerr << "Expected Usage:\n ./assemble infile1.asm infile2.asm ... infilek.asm staticmem_outfile.bin instructions_outfile.bin\n" << std::endl;
        exit(1);
    }
    //Prepare output files
    std::ofstream inst_outfile, static_outfile;
    static_outfile.open(argv[argc - 2], std::ios::binary);
    inst_outfile.open(argv[argc - 1], std::ios::binary);

    //Prepare vectors/iterators for holding information
    std::vector<std::string> instructions;
    int lineCount = 0;
    int staticCount = 0;
    std::vector<std::string> pseudoinsts{"bge", "blt", "bgt", "ble"};
    std::vector<std::string> staticMemoryLines;
    std::map<std::string, int> labels;

    /**
     * Phase 1:
     * Read all instructions, clean them of comments and whitespace DONE
     * TODO: Determine the numbers for all static memory labels
     * (measured in bytes starting at 0)
     * TODO: Determine the line numbers of all instruction line labels
     * (measured in instructions) starting at 0
    */

    //For each input file:
    for (int i = 1; i < argc - 2; i++) {
        std::ifstream infile(argv[i]); //  open the input file for reading
        if (!infile) { // if file can't be opened, need to let the user know
            std::cerr << "Error: could not open file: " << argv[i] << std::endl;
            exit(1);
        }

        std::string str;
        while (getline(infile, str)){ //Read a line from the file
            str = clean(str); // remove comments, leading and trailing whitespace
            if (str == "") { //Ignore empty lines
                continue;
            }

            size_t found = str.find(":"); //Look for colon AKA label in string
            if (found != std::string::npos) { //If a colon is found
                found = str.find("."); //Look for period AKA static memory label
                if (found != std::string::npos) { //If a period is found
                    staticMemoryLines.push_back(str);
                    std::vector<std::string> terms = split(str, WHITESPACE+",()"); //Create string array
                    int valueAmount = terms.size() - 2; //Get the amount of values it is storing in static memory, subtract 2 for label and .word
                    std::string staticLabel = terms[0]; //get label from terms
                    staticLabel.pop_back(); //remove colon from end of label
                    labels[staticLabel] = staticCount*4; //save memory location for this label
                    if (terms[1] == ".asciiz") { //if this is a string
                           
                    }
                    staticCount += valueAmount; //increase static count by number of values so no memory is overwritten
                } else { //Period was not found so this must be a label
                std::string label = str; //get label from string
                label.pop_back(); //remove colon from end of label
                labels[label] = lineCount; //add label to map with line number
                }
            } else { //if a colon is not found AKA it's an instruction
                found = str.find("."); //Look for period AKA static memory label
                if (found != std::string::npos) {
                    continue; //This is a static memory non-label thing so we don't care about it
                } else { //This is not a label or static memory and is thus an instruction
                    instructions.push_back(str);
                    std::vector<std::string> terms = split(str, WHITESPACE+",()"); //Create string array
                    if (std::find(pseudoinsts.begin(), pseudoinsts.end(), terms[0]) != pseudoinsts.end()) { //if first term is a pseudoinstruction (that uses 2 real instructions)
                        lineCount++; //increment line count an extra time since this is two instructions in one
                    }
                    lineCount++;
                }
            }
        }
        infile.close();
    }

    /** Phase 2
     * Process all static memory, output to static memory file
     * TODO: All of this
     */

    for (std::string line : staticMemoryLines) { //for every static memory line
        std::vector<std::string> terms = split(line, WHITESPACE+",()"); //split into terms
        for (int i = 2; i < terms.size(); i++) { //for each item in the list
            if (terms[1] == ".asciiz") { //if the second term is .asciiz AKA if the type of static memory is a string
                size_t strChop = line.find_first_of(':'); //find where the first colon is
                line = line.substr(strChop+11); //remove leading characters from string
                line.pop_back(); //remove ending quotation from string
                for (char letter : line) { //for every character in the string
                    write_binary(int(letter), static_outfile); //write character's ascii to static memory binary
                }
                write_binary(int("\0"), static_outfile); //write end string character's ascii to static memory output
            } else if (labels.count(terms[i]) == 0) { //if terms[i] is not a key in the labels map AKA if this value is not a label
                write_binary(stoi(terms[i]), static_outfile); //write to static memory binary
            } else { //runs when this is a label
                write_binary(labels[terms[i]]*4, static_outfile); //write to static memory binary (multiplied by 4 since it's a label address)
            }
        }
    }   
    /** Phase 3
     * Process all instructions, output to instruction memory file
     * TODO: Almost all of this, it only works for adds
     */
    for(std::string inst : instructions) {
        int lineCount = 0; //reset line count
        std::vector<std::string> terms = split(inst, WHITESPACE+",()");
        std::string inst_type = terms[0];
        if (inst_type == "add") {
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 32),inst_outfile);
        }
        else if (inst_type == "addi") {
            write_binary(encode_Itype(8, registers[terms[2]], registers[terms[1]], stoi(terms[3])),inst_outfile);
        }
        else if (inst_type == "sub") {
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 34),inst_outfile);
        }
        else if (inst_type == "mult") {
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 24),inst_outfile);
        }
        else if (inst_type == "div") {
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], 0, 0, 26),inst_outfile);
        }
        else if (inst_type == "mflo") {
            write_binary(encode_Rtype(0, 0, 0, registers[terms[1]], 0, 18),inst_outfile);
        }
        else if (inst_type == "mfhi") {
            write_binary(encode_Rtype(0, 0, 0, registers[terms[1]], 0, 16),inst_outfile);
        }
        else if (inst_type == "sll") {
            write_binary(encode_Rtype(0, 0, registers[terms[2]], registers[terms[1]], stoi(terms[3]), 0),inst_outfile);
        }
        else if (inst_type == "srl") {
            write_binary(encode_Rtype(0, 0, registers[terms[2]], registers[terms[1]], stoi(terms[3]), 2),inst_outfile);
        }
        else if (inst_type == "slt") {
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 42),inst_outfile);
        }
        else if (inst_type == "lw") {
            write_binary(encode_Itype(35, registers[terms[3]], registers[terms[1]], stoi(terms[2])), inst_outfile);
        }
        else if (inst_type == "sw") {
            write_binary(encode_Itype(43, registers[terms[3]], registers[terms[1]], stoi(terms[2])), inst_outfile);
        }
        else if (inst_type == "j") {
            int target = labels[terms[1]];
            write_binary(encode_Jtype(2, target),inst_outfile);
        }
        else if (inst_type == "jal") {
            int target = labels[terms[1]];
            write_binary(encode_Jtype(3, target),inst_outfile);
        }
        else if (inst_type == "jr") {
            write_binary(encode_Rtype(0, registers[terms[1]], 0, 0, 0, 8),inst_outfile);
        }
        else if (inst_type == "beq") {
            lineCount -= lineCount - (lineCount - labels[terms[3]]);
            write_binary(encode_Itype(4, registers[terms[1]], registers[terms[2]], lineCount),inst_outfile);
        }
        else if (inst_type == "bne") {
            lineCount -= lineCount - (lineCount - labels[terms[3]]);
            write_binary(encode_Itype(5, registers[terms[1]], registers[terms[2]], lineCount),inst_outfile);
        }
        else if (inst_type == "syscall") {
            write_binary(encode_Rtype(0, 0, 0, 26, 0, 12),inst_outfile);
        }
        else if (inst_type == "jalr") {
            write_binary(encode_Rtype(0, registers[terms[1]], 0, 31, 0, 9),inst_outfile);
        }
        else if (inst_type == "la") {
            int address = labels[terms[2]];
            write_binary(encode_Itype(8, registers[terms[2]], registers[terms[1]], address),inst_outfile); //addi
            //write_binary(encode_Itype(15, registers["$at"], 0, loading), inst_outfile); //lui 
            //write_binary(encode_Itype(13, registers[terms[1]], registers["$at"], loading), inst_outfile); //ori 
        }
        else if (inst_type == "move") {
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[3]], registers[terms[1]], 0, 33),inst_outfile); //addu
        }
        else if (inst_type == "li") {
            write_binary(encode_Itype(13, 0, registers[terms[1]], stoi(terms[2])), inst_outfile); //ori
        }
        else if (inst_type == "blt") {
            lineCount -= lineCount - (lineCount - labels[terms[3]]);
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 42),inst_outfile); //slt
            write_binary(encode_Itype(5, registers["$at"], 0, lineCount),inst_outfile); //bne
        }
        else if (inst_type == "ble") {
            lineCount -= lineCount - (lineCount - labels[terms[3]]);
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[1]], registers["$at"], 0, 42),inst_outfile); //slt
            write_binary(encode_Itype(4, registers["$at"], 0, lineCount),inst_outfile); //beq
        }
        else if (inst_type == "bgt") {
            lineCount -= lineCount - (lineCount - labels[terms[3]]);
            write_binary(encode_Rtype(0, registers[terms[2]], registers[terms[1]], registers["$at"], 0, 42),inst_outfile); //slt
            write_binary(encode_Itype(5, registers["$at"], 0, lineCount),inst_outfile); //bne
        }
        else if (inst_type == "bge") {
            lineCount -= lineCount - (lineCount - labels[terms[3]]);
            write_binary(encode_Rtype(0, registers[terms[1]], registers[terms[2]], registers["$at"], 0, 42),inst_outfile); //slt
            write_binary(encode_Itype(4, registers["$at"], 0, lineCount),inst_outfile); //beq
        }
        lineCount++; //increment line count
    }
}
#endif