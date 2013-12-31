#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>
#include <string>

#include <libqalculate/qalculate.h>

using namespace std;

int main()
{
    char expression[2048];
    while(1)
    {
        scanf("%s", expression);
        cout << "Expression: " << expression << endl;
        if(!expression)
            break;
        new Calculator();
        CALCULATOR->loadGlobalDefinitions();
        CALCULATOR->loadLocalDefinitions();
        EvaluationOptions eo;
        MathStructure result = CALCULATOR->calculate(expression, eo);
        PrintOptions po;
        result.format(po);
        string result_str = result.print(po); 
        cout << "Result: " << result_str << endl;
    }
    return 0;
}
