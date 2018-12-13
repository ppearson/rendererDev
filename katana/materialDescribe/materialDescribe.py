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

import os
import math

from Katana import Nodes3DAPI
from Katana import FnAttribute
from Katana import FnGeolibServices

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
    
    def generateMaterialAttributes(self, materialDefinition, materialStaticSCB, materialLocationPath, convertValues):
        shaderNameItem = "imagineSurfaceShader"
        shaderParamsItem = "imagineSurfaceParams"

        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material." + shaderNameItem, FnAttribute.StringAttribute("StandardImage"))

        attrLevelItemPath = "material." + shaderParamsItem + "."

        for matDefName, matDefItem in materialDefinition.iteritems():
            if matDefName == "diffColour":
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diff_col", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diff_col_texture", FnAttribute.StringAttribute(matDefItem[1]))
            elif matDefName == "diffRoughness":
                if matDefItem[0] == "float":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diff_roughness", FnAttribute.FloatAttribute(matDefItem[1]))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diff_roughness_texture", FnAttribute.StringAttribute(matDefItem[1]))
            elif matDefName == "refraIndex" and matDefItem[0] == "float":
                materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "refraction_index", FnAttribute.FloatAttribute(matDefItem[1]))
            elif matDefName == "specColour":
                # Arnold is always GGX, so we'll match that
                materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "microfacet_type", FnAttribute.StringAttribute("ggx"))
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "spec_col", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "spec_col_texture", FnAttribute.StringAttribute(matDefItem[1]))
            elif matDefName == "specRoughness":
                if matDefItem[0] == "float":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "spec_roughness", FnAttribute.FloatAttribute(matDefItem[1]))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "spec_roughness_texture", FnAttribute.StringAttribute(matDefItem[1]))

class OtherPlugin(MaterialPlugin):
    def pluginInfo(self):
        # return name and description
        return ("other", "Other SimpleShaders")
    
    def generateMaterialAttributes(self, materialDefinition, materialStaticSCB, materialLocationPath, convertValues):
        shaderNameItem = "otherSurfaceShader"
        shaderParamsItem = "otherSurfaceParams"

        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material." + shaderNameItem, FnAttribute.StringAttribute("surfaces/basic"))

        attrLevelItemPath = "material." + shaderParamsItem + "."

        for matDefName, matDefItem in materialDefinition.iteritems():
            if matDefName == "diffColour":
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diffuseColor", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diffuseTextureMap", FnAttribute.StringAttribute(matDefItem[1]))
            elif matDefName == "diffRoughness":
                if matDefItem[0] == "float":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diffuseRoughness", FnAttribute.FloatAttribute(matDefItem[1]))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "diffuseRoughnessTextureMap", FnAttribute.StringAttribute(matDefItem[1]))
            elif matDefName == "refraIndex" and matDefItem[0] == "float":
                materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "ior", FnAttribute.FloatAttribute(matDefItem[1]))
            elif matDefName == "specColour":
                # Arnold is always GGX, so we'll match that
                materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "microfacetModel", FnAttribute.FloatAttribute(1.0))
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "specularColor", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "specularTextureMap", FnAttribute.StringAttribute(matDefItem[1]))
            elif matDefName == "specRoughness":
                if matDefItem[0] == "float":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "specularRoughness", FnAttribute.FloatAttribute(matDefItem[1]))
                elif matDefItem[0] == "image":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, attrLevelItemPath + "specularRoughnessTextureMap", FnAttribute.StringAttribute(matDefItem[1]))

class ArnoldPlugin(MaterialPlugin):
    def pluginInfo(self):
        # return name and description
        return ("arnold", "Arnold built-in Standard")

    # again, very nastly, but not much else we can do given the premise of the tool and the differences between
    # renderers' shaders we're not in control of...
    def adjustSpecRoughness(self, roughnessValue):
        # Arnold's shader param values seem to be the Square of the microfacet alpha value
        return sqrt(roughnessValue)

    def addImageShadingNode(self, materialStaticSCB, materialLocationPath, imagePath, nodeName):
        matAttrPath = "material.nodes." + nodeName + "."
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "name", FnAttribute.StringAttribute(nodeName))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "type", FnAttribute.StringAttribute("image"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "target", FnAttribute.StringAttribute("arnold"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "srcName", FnAttribute.StringAttribute(nodeName))

        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "parameters.filename", FnAttribute.StringAttribute(imagePath))
    
    def generateMaterialAttributes(self, materialDefinition, materialStaticSCB, materialLocationPath, convertValues):
        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material.style", FnAttribute.StringAttribute("network"))

        nodeName = "ArnoldShadingNode"
        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material.terminals.arnoldSurface", FnAttribute.StringAttribute(nodeName))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material.terminals.arnoldSurfacePort", FnAttribute.StringAttribute("out"))

        matAttrPath = "material.nodes." + nodeName + "."
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "name", FnAttribute.StringAttribute(nodeName))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "type", FnAttribute.StringAttribute("standard_surface"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "target", FnAttribute.StringAttribute("arnold"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "srcName", FnAttribute.StringAttribute(nodeName))

        imageNodeCount = 1

        matParamsPath = matAttrPath + "parameters."

        for matDefName, matDefItem in materialDefinition.iteritems():
            if matDefName == "diffColour":
                materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "base", FnAttribute.FloatAttribute(1.0))
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "base_color", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    imageNodeName = "ASNImage%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.base_color", FnAttribute.StringAttribute("out@" + imageNodeName))
            elif matDefName == "diffRoughness":
                if matDefItem[0] == "float":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "diffuse_roughness", FnAttribute.FloatAttribute(matDefItem[1]))
                elif matDefItem[0] == "image":
                    imageNodeName = "ASNImage%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.diffuse_roughness", FnAttribute.StringAttribute("out@" + imageNodeName))
            elif matDefName == "refraIndex" and matDefItem[0] == "float":
                materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specular_IOR", FnAttribute.FloatAttribute(matDefItem[1]))
            if matDefName == "specColour":
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specular_color", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    imageNodeName = "ASNImage%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.specular_color", FnAttribute.StringAttribute("out@" + imageNodeName))
            elif matDefName == "specRoughness":
                if matDefItem[0] == "float":
                    actualRoughnessValue = self.adjustSpecRoughness(matDefItem[1]) if convertValues else matDefItem[1]
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specular_roughness", FnAttribute.FloatAttribute(actualRoughnessValue))
                elif matDefItem[0] == "image":
                    imageNodeName = "ASNImage%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.specular_roughness", FnAttribute.StringAttribute("out@" + imageNodeName))

class PRManPlugin(MaterialPlugin):
    def pluginInfo(self):
        # return name and description
        return ("prman", "PRMan PXR built-in Standard")

    # Because Renderman 22 only seem to read Pixar format textures now (or at least ones created with txmake),
    # we need some way to allow it to point at different textures. This obviously makes a bit of a mockery of
    # the whole point of a single neutral Material Definition / Description, but there's not much else we can do,
    # and the other three renderers support EXRs generated with maketx, so we'll special-case PRMan...
    def replacementTexture(self, texturePath):
        return os.path.splitext(texturePath)[0] + ".tex" 
    
    # again, very nastly, but not much else we can do given the premise of the tool and the differences between
    # renderers' shaders we're not in control of...
    def adjustSpecRoughness(self, roughnessValue):
        # PRMan's PXRSurface shader param values seem to be the Square of the microfacet alpha value
        return sqrt(roughnessValue)

    def addImageShadingNode(self, materialStaticSCB, materialLocationPath, imagePath, nodeName):
        matAttrPath = "material.nodes." + nodeName + "."
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "name", FnAttribute.StringAttribute(nodeName))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "type", FnAttribute.StringAttribute("PxrTexture"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "target", FnAttribute.StringAttribute("prman"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "srcName", FnAttribute.StringAttribute(nodeName))

        convertedFilename = self.replacementTexture(imagePath)
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "parameters.filename", FnAttribute.StringAttribute(convertedFilename))
    
    def generateMaterialAttributes(self, materialDefinition, materialStaticSCB, materialLocationPath, convertValues):
        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material.style", FnAttribute.StringAttribute("network"))

        nodeName = "PRManShadingNode"
        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material.terminals.prmanBxdf", FnAttribute.StringAttribute(nodeName))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, "material.terminals.prmanBxdfPort", FnAttribute.StringAttribute("out"))

        matAttrPath = "material.nodes." + nodeName + "."
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "name", FnAttribute.StringAttribute(nodeName))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "type", FnAttribute.StringAttribute("PxrSurface"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "target", FnAttribute.StringAttribute("prman"))
        materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "srcName", FnAttribute.StringAttribute(nodeName))

        imageNodeCount = 1

        matParamsPath = matAttrPath + "parameters."

        for matDefName, matDefItem in materialDefinition.iteritems():
            if matDefName == "diffColour":
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "diffuseColor", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    imageNodeName = "PSNTexture%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.diffuseColor", FnAttribute.StringAttribute("resultRGB@" + imageNodeName))
            elif matDefName == "diffRoughness":
                if matDefItem[0] == "float":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "diffuseRoughness", FnAttribute.FloatAttribute(matDefItem[1]))
                elif matDefItem[0] == "image":
                    imageNodeName = "PSNTexture%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.diffuseRoughness", FnAttribute.StringAttribute("resultA@" + imageNodeName))
            elif matDefName == "refraIndex" and matDefItem[0] == "float":
                tempArrayValue = matDefItem[1] * 3
                materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specularIOR", FnAttribute.FloatAttribute(tempArrayValue, 3))
            elif matDefName == "specColour":
                materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specularFresnelMode", FnAttribute.IntAttribute(1))
                # Set GGX microfacet type
                materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specularModelType", FnAttribute.IntAttribute(1))
                if matDefItem[0] == "col3":
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specularEdgeColor", FnAttribute.FloatAttribute(matDefItem[1], 3))
                elif matDefItem[0] == "image":
                    imageNodeName = "PSNTexture%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.specularEdgeColor", FnAttribute.StringAttribute("resultRGB@" + imageNodeName))
            elif matDefName == "specRoughness":
                if matDefItem[0] == "float":
                    actualRoughnessValue = self.adjustSpecRoughness(matDefItem[1]) if convertValues else matDefItem[1]
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matParamsPath + "specularRoughness", FnAttribute.FloatAttribute(actualRoughnessValue))
                elif matDefItem[0] == "image":
                    imageNodeName = "PSNTexture%i" % (imageNodeCount)
                    imageNodeCount += 1
                    self.addImageShadingNode(materialStaticSCB, materialLocationPath, matDefItem[1], imageNodeName)
                    materialStaticSCB.setAttrAtLocation(materialLocationPath, matAttrPath + "connections.specularRoughness", FnAttribute.StringAttribute("resultA@" + imageNodeName))


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
        baseLocationString = baseLocationParam.getValue(0)

        materialNameParam = node.getParameter('materialName')
        materialNameString = materialNameParam.getValue(0)

        descriptionParam = node.getParameter('description')
        descriptionString = descriptionParam.getValue(0)

        convertValuesParam = node.getParameter('convertValues')
        convertValuesValue = bool(convertValuesParam.getValue(0))

        materialDefinition = parseMaterialDescription(descriptionString)

        rendererListGroupParam = node.getParameter('rendererList')
        renderersToMakeMatsFor = []
        numRendererItems = rendererListGroupParam.getNumChildren()
        for i in range(0, numRendererItems):
            subItemParam = rendererListGroupParam.getChildByIndex(i)
            intValue = subItemParam.getValue(0)
            if intValue == 0:
                continue
            
            rendererName = MaterialPlugin.pluginsNameList[i]
            renderersToMakeMatsFor.append(rendererName)

            materialLocationPath = os.path.join(baseLocationString, rendererName, materialNameString)

            materialStaticSCB = FnGeolibServices.OpArgsBuilders.StaticSceneCreate()
            materialStaticSCB.createEmptyLocation(materialLocationPath)
            materialStaticSCB.setAttrAtLocation(materialLocationPath, 'type', FnAttribute.StringAttribute('material'))

            # invoke the renderer material plugin to create its attributes on the location...
            pluginResult = MaterialPlugin.pluginsDict[rendererName]
            pluginInstance = pluginResult[2]
            pluginInstance.generateMaterialAttributes(materialDefinition, materialStaticSCB, materialLocationPath, convertValuesValue)

            interface.appendOp("StaticSceneCreate", materialStaticSCB.build())
    
    nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('MaterialDescribe')
    
    # build params
    gb = FnAttribute.GroupBuilder()
    gb.set('baseLocation', FnAttribute.StringAttribute('/root/materials/'))
    gb.set('materialName', FnAttribute.StringAttribute('material1'))
    
    gb.set('description', FnAttribute.StringAttribute('diffColour = RGB(0.18);'))

    gb.set('convertValues', FnAttribute.IntAttribute(1))

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