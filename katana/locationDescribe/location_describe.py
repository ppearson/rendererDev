# Created by Peter Pearson in 2017

from Katana import Nodes3DAPI
from Katana import FnAttribute
from Katana import FnGeolibServices

def registerLocationDescribe():
    def buildLocationDescribeOpChain(node, interface):
        interface.setMinRequiredInputs(0)
        
        staticSCB = FnGeolibServices.OpArgsBuilders.StaticSceneCreate()
        
        locationParam = node.getParameter('location')
        if not locationParam:
            # set an error
            staticSCB.createEmptyLocation('/root/world/geo/')
            staticSCB.setAttrAtLocation('/root/world/', 'errorMessage',
                        FnAttribute.StringAttribute('Invalid location used for LocationDescribe'))
        else:
            # do the actual work creating location and attributes
            locationPath = locationParam.getValue(0)
            
            descriptionParam = node.getParameter('description')
            typeParam = node.getParameter('type')
            
            staticSCB.createEmptyLocation(locationPath)
            staticSCB.setAttrAtLocation(locationPath, 'type', FnAttribute.StringAttribute(typeParam.getValue(0)))
            
            descriptionString = descriptionParam.getValue(0)
            attributeItems = parseDescription(descriptionString)
            
            for attribItem in attributeItems:
                if attribItem[0] == 1:
                    # single item
                    if attribItem[1] == "int":
                        staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.IntAttribute(int(attribItem[3])))
                    elif attribItem[1] == "string":
                        staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.StringAttribute(attribItem[3]))
                    elif attribItem[1] == "float":
                        staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.FloatAttribute(float(attribItem[3])))
                    elif attribItem[1] == "double":
                        staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.DoubleAttribute(float(attribItem[3])))
                elif attribItem[0] == 2:
                    # array item
                    itemDataType = attribItem[1]
                    
                    if itemDataType == "int":
                        intArray = [int(stringItem) for stringItem in attribItem[3]]
                        staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.IntAttribute(intArray, attribItem[4]))
                    elif itemDataType == "float" or itemDataType == "double":
                        floatArray = [float(stringItem.translate(None, 'f')) for stringItem in attribItem[3]]
                        # print floatArray
                        if itemDataType == "float":
                            staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.FloatAttribute(floatArray, attribItem[4]))
                        elif itemDataType == "double":
                            staticSCB.setAttrAtLocation(locationPath, attribItem[2], FnAttribute.DoubleAttribute(floatArray, attribItem[4]))
                    
        interface.appendOp('StaticSceneCreate', staticSCB.build())

    nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('LocationDescribe')
    
    # build params
    gb = FnAttribute.GroupBuilder()
    gb.set('location', FnAttribute.StringAttribute('/root/world/geo/location1'))
    gb.set('type', FnAttribute.StringAttribute('sphere'))
    gb.set('description', FnAttribute.StringAttribute('double geometry.radius = 1.0;'))
    
    nodeTypeBuilder.setParametersTemplateAttr(gb.build())
    nodeTypeBuilder.setHintsForParameter('description', {'widget':'scriptEditor'})
    
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
