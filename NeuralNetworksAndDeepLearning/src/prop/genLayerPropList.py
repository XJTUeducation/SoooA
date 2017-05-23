#!/usr/bin/env python

"""genLayerProp.py: """

import json;

####################################### Modify here ##########################################
# if you want to use specific custom type, you should insert header file that the custom type 
# is defined into headerFileList.
headerFileList = ["LayerConfig.h", "LossLayer.h"]
##############################################################################################

# XXX: source code refactoring is required

def checkParamProperty(propDic, prop, propertyName):
    if not propertyName in propDic[prop]:
        print "ERROR: prop %s does not have %s property" % (prop, propertyName)
        exit(-1)

# (1) load propDef.json
try:
    jsonFile = open('layerPropDef.json', 'r')
    propDic = json.load(jsonFile)

except Exception as e:
    print str(e)
    exit(-1)

finally:
    jsonFile.close()

# (2) check propDef syntax
for prop in propDic:
    checkParamProperty(propDic, prop, "DESC")
    checkParamProperty(propDic, prop, "PARENT")
    checkParamProperty(propDic, prop, "LEVEL")
    checkParamProperty(propDic, prop, "LEARN")
    checkParamProperty(propDic, prop, "VARS")

# (3) generate header file
headerTopSentences = [\
"/**",\
" * @file LayerPropList.h",\
" * @author moonhoen lee",\
" * @brief layer property structure list module",\
" * @warning",\
" *  The file is auto-generated.",\
" *  Do not modify the file!!!!",\
" */",\
"",\
"#ifndef LAYERPROPLIST_H_",\
"#define LAYERPROPLIST_H_",\
"",\
"#include <stdint.h>",\
"#include <string.h>",\
"#include <vector>",\
"#include <string>",\
"",\
'#include "common.h"',\
'#include "SysLog.h"',\
'#include "LayerProp.h"',\
"",\
]

headerClassDefSentences = [\
"class LayerPropList {",\
"public : ",\
"    LayerPropList() {}",\
"    virtual ~LayerPropList() {}",\
]

headerBottomSentences = [\
"};",\
"",\
"#endif /* LAYERPROPLIST_H_ */",\
]

# (4) generate source file
sourceTopSentences = [\
"/**",\
" * @file LayerPropList.cpp",\
" * @author moonhoen lee",\
" * @brief layer property structure list module",\
" * @warning",\
" *  The file is auto-generated.",\
" *  Do not modify the file!!!!",\
" */",\
"",\
'#include "LayerPropList.h"',\
'#include "SysLog.h"',\
'#include "LayerFunc.h"',\
"",\
"",\
]

varDic = dict()     # key : layer name(prop), value : vars
levelDic = dict()   # key : level, layer name(prop) list
maxLevel = -1

try:
    headerFile = open('LayerPropList.h', 'w+')
    sourceFile = open('LayerPropList.cpp', 'w+')

    # write top sentences
    for line in headerTopSentences:
        headerFile.write(line + "\n")

    for headerFileName in headerFileList:
        headerFile.write('#include "%s"\n' % headerFileName)
    headerFile.write('\n')

    for line in sourceTopSentences:
        sourceFile.write(line + "\n")

    # fill levelDic
    for prop in propDic:
        # (1) parse property structure def
        level = propDic[prop]["LEVEL"]
        if level not in levelDic:
            levelDic[level] = []

        levelDic[level].append(prop)

        if level > maxLevel:
            maxLevel = level
    
    # write structure
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in varDic:
                print "ERROR: duplicate prop(layer name). prop=%s" % prop
                exit(-1)

            varDic[prop] = []
           
            # fill parent var
            parent = propDic[prop]["PARENT"]
            if len(parent) > 1:
                if parent not in varDic:
                    print "ERROR: specified parent is not defined. prop=%s, parent=%s"\
                        % (prop, parent)
                    exit(-1)


                for var in varDic[parent]:
                    varDic[prop].append(var)

            for var in propDic[prop]["VARS"]:
                varDic[prop].append(var) 

            # (2) generate comment for property layer name
            headerFile.write('// property layer name : %s\n' % prop)

            headerFile.write('typedef struct %sPropLayer_s {\n' % prop)
            for var in varDic[prop]:
                if 'vector' in var[1]:
                    headerFile.write('    %s _%s_;\n' % (var[1], var[0]))
                elif 'char[' in var[1]:
                    splited = var[1].replace(']', '@').replace('[', '@').split('@')
                    headerFile.write('    %s _%s_[%s];\n' % (splited[0], var[0], splited[1]))
                elif 'string' in var[1]:
                    headerFile.write('    %s _%s_;\n' % (var[1], var[0]))
                else:
                    headerFile.write('    %s _%s_;\n' % (var[1], var[0]))

            headerFile.write('\n    %sPropLayer_s() {\n' % prop)

            for var in varDic[prop]:
                if var[0] == 'learnable':
                    if propDic[prop]["LEARN"] == True:
                        headerFile.write('        _%s_ = true;\n' % var[0])
                    else:
                        headerFile.write('        _%s_ = false;\n' % var[0])
                elif 'vector' in var[1]:
                    headerFile.write('        _%s_ = {%s};\n' % (var[0], var[2]))
                elif 'char[' in var[1]:
                    headerFile.write('        strcpy(_%s_, %s);\n' % (var[0], var[2]))
                elif 'string' in var[1]:
                    headerFile.write('        _%s_ = %s;\n' % (var[0], var[2]))
                else:
                    headerFile.write('        _%s_ = (%s)%s;\n' % (var[0], var[1], var[2]))
            headerFile.write('\n    }\n')

            headerFile.write('} _%sPropLayer;\n\n' % prop)

  
    # write class
    for line in headerClassDefSentences:
        headerFile.write(line + "\n")

    # declare functions
    headerFile.write("    static void setProp(void* target, const char* layer,")
    headerFile.write(" const char* property, void* value);\n")
    headerFile.write("    static LayerProp* createLayerProp(int networkID, int layerID,")
    headerFile.write(" const char* layerName);\n")
    headerFile.write("    static int getLayerType(const char* layer);\n")
    headerFile.write("    static std::vector<std::string> getInputs(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static std::vector<std::string> getOutputs(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static std::vector<bool> getPropDowns(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static bool isDonator(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static bool isReceiver(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static int getDonatorID(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static bool isLearnable(")
    headerFile.write("const char* layer, void* target);\n")
    headerFile.write("    static void init();\n\n")
    headerFile.write("private:\n")

    # set property functions
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            headerFile.write(\
                "    static void set%s(void* target, const char* property, void* value);\n"\
                % prop)
            sourceFile.write(\
            "void LayerPropList::set%s(void* target, const char* property, void* value) {\n"\
                % prop)
            sourceFile.write('    _%sPropLayer* obj = (_%sPropLayer*)target;\n\n'\
                % (prop, prop))

            isFirstCond = True

            for var in varDic[prop]:
                if isFirstCond:
                    sourceFile.write('    if (strcmp(property, "%s") == 0) {\n' % var[0])
                    isFirstCond = False
                else:
                    sourceFile.write(' else if (strcmp(property, "%s") == 0) {\n' % var[0])

                if 'vector' in var[1]:
                    subType = var[1].replace('<', '').replace('>', '').split('vector')[1]
                    if 'string' in subType:
                        sourceFile.write('        std::vector<std::string> *val = ')
                        sourceFile.write('(std::vector<std::string>*)value;\n')
                    elif subType in ['int', 'unsigned int', 'int32_t', 'uint32_t',\
                        'int64_t', 'uint64_t', 'long', 'unsigned long', 'short',\
                        'unsigned short', 'long long', 'unsigned long long']:
                        sourceFile.write('        std::vector<int64_t> *val = ')
                        sourceFile.write('(std::vector<int64_t>*)value;\n')
                    elif subType in ['boolean', 'bool']:
                        sourceFile.write('        std::vector<bool> *val = ')
                        sourceFile.write('(std::vector<bool>*)value;\n')
                    elif subType in ['double', 'float']:
                        sourceFile.write('        std::vector<double> *val = ')
                        sourceFile.write('(std::vector<double>*)value;\n')
                    else:
                        print 'unsupported subtype for array. subtype = %s' % subType
                        exit(-1)

                    sourceFile.write('        for (int i = 0; i < (*val).size(); i++) {\n')
                    sourceFile.write('            obj->_%s_.push_back((%s)(*val)[i]);\n'\
                        % (var[0], subType))
                    sourceFile.write('        }\n')
                elif 'char[' in var[1]:
                    sourceFile.write('        strcpy(obj->_%s_, (const char*)value);\n'\
                        % var[0])
                elif 'string' in var[1]:
                    sourceFile.write('        std::string* val = (std::string*)value;\n')
                    sourceFile.write('        obj->_%s_ = *val;\n' % var[0])
                else:
                    sourceFile.write('        %s* val = (%s*)value;\n' % (var[1], var[1]))
                    sourceFile.write('        obj->_%s_ = *val;\n' % var[0])
                sourceFile.write('    }')
            sourceFile.write(' else {\n')
            sourceFile.write('        SASSERT(false, "invalid property.')
            sourceFile.write(' layer name=%s, property=%s"')
            sourceFile.write(', "%s", property);\n    }\n}\n\n' % prop) 

    # setProp() function
    sourceFile.write("void LayerPropList::setProp(void *target, const char* layer,")
    sourceFile.write(" const char* property, void* value) {\n")
   
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        set%s(target, property, value);\n' % prop) 
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    # create layer prop function
    sourceFile.write("LayerProp* LayerPropList::createLayerProp(int networkID, int layerID,")
    sourceFile.write(" const char* layerName) {\n")
   
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layerName, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layerName, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = new _%sPropLayer();\n'\
                % (prop, prop))
            sourceFile.write('        return new LayerProp(networkID, layerID,')
            sourceFile.write(' (int)Layer<float>::%s, (void*)prop);\n' % prop)
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layerName);\n    }\n}\n\n')

    # get layer type function
    sourceFile.write("int LayerPropList::getLayerType(const char* layer) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        return (int)Layer<float>::%s;\n' % prop)
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    # inputs, outputs, propDownds, isDonator, isReceiver, getDonatorID, isLearnable functions
    sourceFile.write("std::vector<std::string> LayerPropList::getInputs")
    sourceFile.write("(const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_input_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    sourceFile.write("std::vector<std::string> LayerPropList::getOutputs(")
    sourceFile.write("const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_output_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    sourceFile.write("std::vector<bool> LayerPropList::getPropDowns")
    sourceFile.write("(const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_propDown_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    sourceFile.write("bool LayerPropList::isDonator")
    sourceFile.write("(const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_donate_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    sourceFile.write("bool LayerPropList::isReceiver")
    sourceFile.write("(const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_receive_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    sourceFile.write("int LayerPropList::getDonatorID")
    sourceFile.write("(const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_donatorID_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

    sourceFile.write("bool LayerPropList::isLearnable")
    sourceFile.write("(const char* layer, void* target) {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            if isFirstCond:
                sourceFile.write('    if (strcmp(layer, "%s") == 0) {\n' % prop)
                isFirstCond = False
            else:
                sourceFile.write(' else if (strcmp(layer, "%s") == 0) {\n' % prop)
            sourceFile.write('        _%sPropLayer *prop = (_%sPropLayer*)target;\n'\
                % (prop, prop))
            sourceFile.write('        return prop->_learnable_;\n')
            sourceFile.write('    }')
    sourceFile.write(' else {\n')
    sourceFile.write('        SASSERT(false, "invalid layer. layer name=%s"')
    sourceFile.write(', layer);\n    }\n}\n\n')

  
    # include files for init()
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop == 'Base':
                continue

            sourceFile.write('#include "%sLayer.h"\n' % prop)

    # init() function
    sourceFile.write("\nvoid LayerPropList::init() {\n")
    isFirstCond = True
    for level in range(maxLevel + 1):
        propList = levelDic[level]

        for prop in propList:
            if prop in ['Base', 'Loss', 'Learnable']:
                continue

            sourceFile.write('    LayerFunc::registerLayerFunc(')
            sourceFile.write('(int)Layer<float>::%s, ' % prop)
            sourceFile.write('%sLayer<float>::initLayer, ' % prop)
            sourceFile.write('%sLayer<float>::destroyLayer, ' % prop)
            sourceFile.write('%sLayer<float>::setInOutTensor, ' % prop)
            sourceFile.write('%sLayer<float>::allocLayerTensors, ' % prop)
            sourceFile.write('%sLayer<float>::forwardTensor, ' % prop)
            sourceFile.write('%sLayer<float>::backwardTensor, ' % prop)
            sourceFile.write('%sLayer<float>::learnTensor);\n' % prop)
    sourceFile.write('\n};\n\n')

    # write header bottom
    for line in headerBottomSentences:
        headerFile.write(line + "\n")

except Exception as e:
    print str(e)
    exit(-1)

finally:
    headerFile.close()
