# Created by Peter Pearson in 2017

from Katana import Nodes3DAPI
from Katana import FnAttribute
from Katana import FnGeolibServices

class InternalBuilderSSC:
    def __init__(self, locationPath, locationType):
        self.locationPath = locationPath
        self.locationType = locationType
        self.staticSCB = FnGeolibServices.OpArgsBuilders.StaticSceneCreate()
        self.staticSCB.createEmptyLocation(self.locationPath)
        self.staticSCB.setAttrAtLocation(self.locationPath, 'type', FnAttribute.StringAttribute(self.locationType))
    def addAttribute(self, attributeName, attribute):
        self.staticSCB.setAttrAtLocation(self.locationPath, attributeName, attribute)
    def getOpName(self):
        return "StaticSceneCreate"
    def build(self):
        return self.staticSCB.build()

class InternalBuilderAS:
    def __init__(self, locationPath, locationType):
        self.locationPath = locationPath
        self.locationType = locationType
        self.attribSet = FnGeolibServices.OpArgsBuilders.AttributeSet()
        self.attribSet.setCEL(FnAttribute.StringAttribute(self.locationPath))
    def addAttribute(self, attributeName, attribute):
        self.attribSet.setAttr(attributeName, attribute)
    def getOpName(self):
        return "AttributeSet"
    def build(self):
        return self.attribSet.build()

def registerLocationDescribe():
    def buildLocationDescribeOpChain(node, interface):
        interface.setMinRequiredInputs(0)
        
        locationParam = node.getParameter('location')
        if not locationParam:
            # set an error
            staticSCB = FnGeolibServices.OpArgsBuilders.StaticSceneCreate()
            staticSCB.createEmptyLocation('/root/world/geo/')
            staticSCB.setAttrAtLocation('/root/world/', 'errorMessage',
                        FnAttribute.StringAttribute('Invalid location used for LocationDescribe'))
            interface.appendOp('StaticSceneCreate', staticSCB.build())
        else:
            # do the actual work creating location and attributes
            locationPath = locationParam.getValue(0)
            
            modeParam = node.getParameter('mode')
            modeParamValue = modeParam.getValue(0)
            descriptionParam = node.getParameter('description')
            typeParam = node.getParameter('type')
            typeParamValue = typeParam.getValue(0)
            
            interface.setMinRequiredInputs(0 if modeParamValue == "create" else 1)
            
            internalBuilder = InternalBuilderSSC(locationPath, typeParamValue) if modeParamValue == "create" else InternalBuilderAS(locationPath, typeParamValue)
            
            descriptionString = descriptionParam.getValue(0)
            attributeItems = parseDescription(descriptionString)
            
            for attribItem in attributeItems:
                if attribItem[0] == 1:
                    # single item
                    if attribItem[1] == "int":
                        internalBuilder.addAttribute(attribItem[2], FnAttribute.IntAttribute(int(attribItem[3])))
                    elif attribItem[1] == "string":
                        internalBuilder.addAttribute(attribItem[2], FnAttribute.StringAttribute(attribItem[3]))
                    elif attribItem[1] == "float":
                        internalBuilder.addAttribute(attribItem[2], FnAttribute.FloatAttribute(float(attribItem[3])))
                    elif attribItem[1] == "double":
                        internalBuilder.addAttribute(attribItem[2], FnAttribute.DoubleAttribute(float(attribItem[3])))
                elif attribItem[0] == 2:
                    # array item
                    itemDataType = attribItem[1]
                    
                    if itemDataType == "int":
                        intArray = [int(stringItem) for stringItem in attribItem[3]]
                        internalBuilder.addAttribute(attribItem[2], FnAttribute.IntAttribute(intArray, attribItem[4]))
                    elif itemDataType == "float" or itemDataType == "double":
                        floatArray = [float(stringItem.translate(None, 'f')) for stringItem in attribItem[3]]
                        # print floatArray
                        if itemDataType == "float":
                            internalBuilder.addAttribute(attribItem[2], FnAttribute.FloatAttribute(floatArray, attribItem[4]))
                        elif itemDataType == "double":
                            internalBuilder.addAttribute(attribItem[2], FnAttribute.DoubleAttribute(floatArray, attribItem[4]))
                    
            interface.appendOp(internalBuilder.getOpName(), internalBuilder.build())

    nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('LocationDescribe')
    
    # build params
    gb = FnAttribute.GroupBuilder()
    gb.set('location', FnAttribute.StringAttribute('/root/world/geo/location1'))
    gb.set('mode', FnAttribute.StringAttribute('create'))
    gb.set('type', FnAttribute.StringAttribute('sphere'))
    gb.set('description', FnAttribute.StringAttribute('double geometry.radius = 1.0;'))
    
    nodeTypeBuilder.setParametersTemplateAttr(gb.build())
    nodeTypeBuilder.setHintsForParameter('mode', {'widget':'popup', 'options':'create|edit'})
    nodeTypeBuilder.setHintsForParameter('description', {'widget':'scriptEditor'})
    
    nodeTypeBuilder.setInputPortNames(('in',))
    
    nodeTypeBuilder.setBuildOpChainFnc(buildLocationDescribeOpChain)
    
    nodeTypeBuilder.build()
    
# returns a parsed list of tuple triplets of each attribute, containing the type, name and value as strings
def parseDescription(descriptionString):
    attributes = []
    
    descriptionLines = descriptionString.splitlines()
    
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
        
        # up to first space is the data type of the attribute to create
        typeSep = line.find(" ")
        if typeSep == -1:
            continue
        
        attributeDataType = line[:typeSep]
        
        # indicator of whether it's a single attribute or an array attribute.
        # 1 if single attribute, 2 if an array
        attributeItemType = 2 if attributeDataType.find("[") != -1 else 1
        
        # find end of attribute name
        attributeNameEndSep = line.find(" = ")
        if attributeNameEndSep == -1:
            continue
        
        attributeName = line[typeSep:attributeNameEndSep]
        attributeName = attributeName.strip()
        
        attributeValue = line[attributeNameEndSep + 2:]
        attributeValue = attributeValue.strip()
        
        if attributeItemType == 1:
            # just a single value for the attribute
            attributes.append((attributeItemType, attributeDataType, attributeName, attributeValue))
        elif attributeItemType == 2:
            # an array type for the attribute
            if attributeValue[0] != "{" or attributeValue[-1:] != "}":
                print "Invalid array value string for attribute: %s" % (attributeName)
                continue
            
            # strip the braces off
            tempValueString = attributeValue[1:-1]
            tempValueString = tempValueString.strip()
            
            tempValueString = tempValueString.replace(", ", ",")
            tempArrayItems = tempValueString.split(",")
            
            baseTypeSep = attributeDataType.find("[")
            baseDataType = attributeDataType[:baseTypeSep]
            
            arrayEndSep = attributeDataType.find("]")
            tupleSize = 1
            if arrayEndSep > baseTypeSep + 1:
                tupleSizeString = attributeDataType[baseTypeSep + 1:arrayEndSep]
                tupleSize = int(tupleSizeString)
            
            attributes.append((attributeItemType, baseDataType, attributeName, tempArrayItems, tupleSize))
    
    return attributes

registerLocationDescribe()
