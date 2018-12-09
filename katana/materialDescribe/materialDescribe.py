'''
 MaterialDescribe
 Copyright 2018 Peter Pearson.

 Licensed under the Apache License, Version 2.0 (the "License");
 You may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ---------
'''

from Katana import Nodes3DAPI
from Katana import FnAttribute
#from Katana import FnGeolibServices

# plugin stuff - should be in separate files at some point...

class MaterialPluginBase(type):
    def __init__(cls, name, bases, attrs):
        if not hasattr(cls, 'pluginsDict'):
            # first time around
            cls.pluginsNameList = []
            cls.pluginsDict = {}
        else:
            # derived class is called
            cls.registerPlugin(cls)
    
    def registerPlugin(cls, plugin):
        instance = plugin()
        pluginInfo = instance.pluginInfo()
        cls.pluginsNameList.append(pluginInfo[0])
        cls.pluginsDict[pluginInfo[0]] = (pluginInfo[0], pluginInfo[1], instance)

class MaterialPlugin(object):
    __metaclass__ = MaterialPluginBase

class ImaginePlugin(MaterialPlugin):
    def pluginInfo(self):
        # return name and description
        return ("imagine", "Imagine built-in")
    
    def generateMaterialAttributes(self, materialDefinition):
        return None

class ArnoldPlugin(MaterialPlugin):
    def pluginInfo(self):
        # return name and description
        return ("arnold", "Arnold built-in Standard")
    
    def generateMaterialAttributes(self, materialDefinition):
        return None

def processSimpleDefinition(definitionValueString):
    if definitionValueString.find('(') == -1:
        return ("float", float(definitionValueString))
    else:
        # it's a function of some sort
        parenthStartIndex = definitionValueString.find('(')
        parenthEndIndex = definitionValueString.find(')', parenthStartIndex + 1)

        functionName = definitionValueString[:parenthStartIndex]
        functionArgument = definitionValueString[parenthStartIndex + 1:parenthEndIndex]

        if functionName == "RGB":
            functionArgument = functionArgument.translate(None, ' ')
            argumentItems = functionArgument.split(",")
            if len(argumentItems) == 1:
                return ("col3", map(float, argumentItems * 3))
            elif len(argumentItems) == 3:
                return ("col3", map(float, argumentItems))
            else:
                return ("invalid", (functionName, functionArgument))
        elif functionName == "Image":
            # strip off quotes...
            imagePath = functionArgument[1:-1]
            return ("image", imagePath)

        return ("unknown", functionArgument)

def parseStatement(statementString):
    assignmentSep = statementString.find(" = ")
    if assignmentSep == -1:
        return None
    
    definitionName = statementString[:assignmentSep]
    definitionName = definitionName.strip()
    
    definitionValue = statementString[assignmentSep + 2:]
    definitionValue = definitionValue.strip()

    definitionItems = []

    numParentheses = definitionValue.count('(')
    if numParentheses <= 1:
        definitionItems = processSimpleDefinition(definitionValue)
    else:
        definitionItems = processAdvancedDefinition(definitionValue)

    return (definitionName, definitionItems)

def parseMaterialDescription(descriptionString):
    descriptionLines = descriptionString.splitlines()

    materialDef = {}
    
    for line in descriptionLines:
        # strip chars we don't care about
        line = line.translate(None, ';')
        # strip whitespace at the ends
        line = line.strip()
        
        # and blank lines
        if not len(line):
            continue
        
        # skip commented lines
        if line[0] == "#" or line[0] == "/":
            continue
        
        statementResult = parseStatement(line)
        if statementResult is not None:
            materialDef[statementResult[0]] = statementResult[1]
    
    return materialDef

def registerMaterialDescribe():
    def buildMaterialDescribeOpChain(node, interface):
        interface.setMinRequiredInputs(0)

        baseLocationParam = node.getParameter('baseLocation')

        materialNameParam = node.getParameter('materialName')

        descriptionParam = node.getParameter('description')
        descriptionString = descriptionParam.getValue(0)

        materialDescription = parseMaterialDescription(descriptionString)
        print materialDescription

        rendererListGroupParam = node.getParameter('rendererList')
        renderersToMakeMatsFor = []
        numRendererItems = rendererListGroupParam.getNumChildren()
        for i in range(0, numRendererItems):
            subItemParam = rendererListGroupParam.getChildByIndex(i)
            intValue = subItemParam.getValue(0)
            if intValue == 1:
                renderersToMakeMatsFor.append(MaterialPlugin.pluginsNameList[i])
        
        print renderersToMakeMatsFor
    
    nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('MaterialDescribe')
    
    # build params
    gb = FnAttribute.GroupBuilder()
    gb.set('baseLocation', FnAttribute.StringAttribute('/root/materials/'))
    gb.set('materialName', FnAttribute.StringAttribute('material1'))
    
    gb.set('description', FnAttribute.StringAttribute('diffColour = RGB(0.18);'))

    rendererListGb = FnAttribute.GroupBuilder()
    for pluginName in MaterialPlugin.pluginsNameList:
        rendererListGb.set(pluginName, FnAttribute.IntAttribute(1))
    gb.set('rendererList', rendererListGb.build())
    
    nodeTypeBuilder.setParametersTemplateAttr(gb.build())
    nodeTypeBuilder.setHintsForParameter('description', {'widget':'scriptEditor'})

    for pluginName in MaterialPlugin.pluginsNameList:
        nodeTypeBuilder.setHintsForParameter('rendererList.' + pluginName, {'widget':'checkBox'})
    
    nodeTypeBuilder.setInputPortNames(('in',))
    
    nodeTypeBuilder.setBuildOpChainFnc(buildMaterialDescribeOpChain)
    
    nodeTypeBuilder.build()


registerMaterialDescribe()