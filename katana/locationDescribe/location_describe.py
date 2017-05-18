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
                if attribItem[0] == "int":
                    staticSCB.setAttrAtLocation(locationPath, attribItem[1], FnAttribute.IntAttribute(int(attribItem[2])))
                elif attribItem[0] == "string":
                    staticSCB.setAttrAtLocation(locationPath, attribItem[1], FnAttribute.StringAttribute(attribItem[2]))
                elif attribItem[0] == "float":
                    staticSCB.setAttrAtLocation(locationPath, attribItem[1], FnAttribute.FloatAttribute(float(attribItem[2])))
                elif attribItem[0] == "double":
                    staticSCB.setAttrAtLocation(locationPath, attribItem[1], FnAttribute.DoubleAttribute(float(attribItem[2])))
                    
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
        
        # skip commented lines
        if line[0] == "#" or line[0] == "/":
            continue
        
        # and blank lines
        if not len(line):
            continue
        
        # up to first space is the type
        typeSep = line.find(" ")
        if typeSep == -1:
            continue
        
        attributeType = line[:typeSep]
        
        # find end of attribute name
        attributeNameEndSep = line.find(" = ")
        if attributeNameEndSep == -1:
            continue
        
        attributeName = line[typeSep:attributeNameEndSep]
        attributeName = attributeName.strip()
        
        attributeValue = line[attributeNameEndSep + 2:]
        attributeValue = attributeValue.strip()
        
        attributes.append((attributeType, attributeName, attributeValue))
    
    return attributes

registerLocationDescribe()
