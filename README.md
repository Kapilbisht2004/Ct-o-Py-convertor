To execute the file first clone it locally 
Then open folder in VScode 

then run this command ------>   g++ -std=c++17 main.cpp Lexer.cpp Parser.cpp transpiler.cpp -o transpiler
then the transpiler.exe will be generated.
before this pls install and run this command ------->  pip install PyQt5
now run this command ------->   python gui.py

you will see list to tokens 
and after that you will see Abstract syntax tree
and now the python code generated will also be shown in GUI.

Now to test the functioning of project make changes to input_code.c file.
the output will be saved to Converted.py