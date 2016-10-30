from Katana import Nodes3DAPI
from Katana import FnAttribute

def buildInstancerOpChain(node, interface):
	interface.setMinRequiredInputs(0)
	argsGb = FnAttribute.GroupBuilder()

	targetLocationParam = node.getParameter('targetLocation')
	sourceLocationParam = node.getParameter('sourceLocation')
	numInstancesParam = node.getParameter('numInstances')
	areaSpreadParam = node.getParameter('areaSpread')
	groupInstancesParam = node.getParameter('groupInstances')
	groupSizeParam = node.getParameter('groupSize')
	if targetLocationParam:
		location = targetLocationParam.getValue(0)

		locationPaths = location[1:].split('/')[1:]
		attrsHierarchy = 'c.' + '.c.'.join(locationPaths)

		argsGb.set(attrsHierarchy + '.a.sourceLocation', FnAttribute.StringAttribute(sourceLocationParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.numInstances', FnAttribute.IntAttribute(numInstancesParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.areaSpread', FnAttribute.FloatAttribute(areaSpreadParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.groupInstances', FnAttribute.IntAttribute(groupInstancesParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.groupSize', FnAttribute.IntAttribute(groupSizeParam.getValue(0)))

	interface.appendOp('Instancer', argsGb.build())

nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('Instancer')

gb = FnAttribute.GroupBuilder()
gb.set('targetLocation', FnAttribute.StringAttribute('/root/world/geo/instancer1'))
gb.set('sourceLocation', FnAttribute.StringAttribute(''))
gb.set('numInstances', FnAttribute.IntAttribute(100))
gb.set('areaSpread', FnAttribute.FloatAttribute(30.0))
gb.set('groupInstances', FnAttribute.IntAttribute(0))
gb.set('groupSize', FnAttribute.IntAttribute(400))

nodeTypeBuilder.setParametersTemplateAttr(gb.build())

nodeTypeBuilder.setHintsForParameter('targetLocation', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('sourceLocation', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('numInstances', {'int' : True})
nodeTypeBuilder.setHintsForParameter('groupInstances', {'widget' : 'checkBox'})
nodeTypeBuilder.setHintsForParameter('groupSize', {'int' : True, 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../groupInstances', 'conditionalVisValue' : '1'})

nodeTypeBuilder.setBuildOpChainFnc(buildInstancerOpChain)
nodeTypeBuilder.build()
