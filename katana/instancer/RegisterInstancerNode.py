'''
 InstancesCreate
 Copyright 2016-2019 Peter Pearson.

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

def buildInstancesCreateOpChain(node, interface):
	interface.setMinRequiredInputs(0)
	argsGb = FnAttribute.GroupBuilder()

	targetLocationParam = node.getParameter('targetLocation')
	sourceLocationParam = node.getParameter('sourceLocation')
	createdShapeTypeParam = node.getParameter('createdShapeType')
	positionsFilePathParam = node.getParameter('positionsFilePath')
	numInstancesParam = node.getParameter('numInstances')
	instanceArrayParam = node.getParameter('instanceArray')
	createInstanceIndexAttributeParam = node.getParameter('createInstanceIndexAttribute')
	
	floatFormatMatrixParam = node.getParameter('floatFormatMatrix')
	areaSpreadParamX = node.getParameter('areaSpread.i0')
	areaSpreadParamY = node.getParameter('areaSpread.i1')
	areaSpreadParamZ = node.getParameter('areaSpread.i2')
	groupInstancesParam = node.getParameter('groupInstances')
	groupSizeParam = node.getParameter('groupSize')
	if targetLocationParam:
		location = targetLocationParam.getValue(0)

		locationPaths = location[1:].split('/')[1:]
		attrsHierarchy = 'c.' + '.c.'.join(locationPaths)

		argsGb.set(attrsHierarchy + '.a.sourceLocation', FnAttribute.StringAttribute(sourceLocationParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.createdShapeType', FnAttribute.IntAttribute(createdShapeTypeParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.positionsFilePath', FnAttribute.StringAttribute(positionsFilePathParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.numInstances', FnAttribute.IntAttribute(numInstancesParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.instanceArray', FnAttribute.IntAttribute(instanceArrayParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.createInstanceIndexAttribute', FnAttribute.IntAttribute(createInstanceIndexAttributeParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.floatFormatMatrix', FnAttribute.IntAttribute(floatFormatMatrixParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.areaSpread', FnAttribute.FloatAttribute([areaSpreadParamX.getValue(0), areaSpreadParamY.getValue(0), areaSpreadParamZ.getValue(0)], 3))
		argsGb.set(attrsHierarchy + '.a.groupInstances', FnAttribute.IntAttribute(groupInstancesParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.groupSize', FnAttribute.IntAttribute(groupSizeParam.getValue(0)))

	interface.appendOp('InstancesCreate', argsGb.build())

nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('InstancesCreate')

gb = FnAttribute.GroupBuilder()
gb.set('targetLocation', FnAttribute.StringAttribute('/root/world/geo/instances1'))
gb.set('createdShapeType', FnAttribute.IntAttribute(0))
gb.set('sourceLocation', FnAttribute.StringAttribute(''))
gb.set('positionsFilePath', FnAttribute.StringAttribute(''))
gb.set('numInstances', FnAttribute.IntAttribute(1000))
gb.set('instanceArray', FnAttribute.IntAttribute(1))
gb.set('createInstanceIndexAttribute', FnAttribute.IntAttribute(0))
gb.set('floatFormatMatrix', FnAttribute.IntAttribute(0))
gb.set('areaSpread', FnAttribute.FloatAttribute([200.0, 200.0, 200.0], 3))
gb.set('groupInstances', FnAttribute.IntAttribute(0))
gb.set('groupSize', FnAttribute.IntAttribute(400))

nodeTypeBuilder.setParametersTemplateAttr(gb.build())

nodeTypeBuilder.setHintsForParameter('targetLocation', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('createdShapeType', {'widget' : 'mapper',
				 'options' : '2D grid:0|3D grid:1|positions read from ASCII file:2|positions read from binary file:3'})
nodeTypeBuilder.setHintsForParameter('sourceLocation', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('positionsFilePath', {'widget' : 'assetIdInput',
				 'conditionalVisOp' : 'greaterThan', 'conditionalVisPath' : '../createdShapeType', 'conditionalVisValue' : '1'})
nodeTypeBuilder.setHintsForParameter('instanceArray', {'widget' : 'checkBox'})
nodeTypeBuilder.setHintsForParameter('createInstanceIndexAttribute', {'widget' : 'checkBox',
			'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../instanceArray', 'conditionalVisValue' : '1',
			'help' : 'Some renderers require an attribute pointing to the index of the sourceGeo.'})
nodeTypeBuilder.setHintsForParameter('floatFormatMatrix', {'widget' : 'checkBox'})
nodeTypeBuilder.setHintsForParameter('numInstances', {'int' : True, 'conditionalVisOp' : 'lessThan', 'conditionalVisPath' : '../createdShapeType', 'conditionalVisValue' : '2'})
nodeTypeBuilder.setHintsForParameter('areaSpread', {'conditionalVisOp' : 'lessThan', 'conditionalVisPath' : '../createdShapeType', 'conditionalVisValue' : '2'})
nodeTypeBuilder.setHintsForParameter('groupInstances', {'widget' : 'checkBox', 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../instanceArray', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('groupSize', {'int' : True, 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../groupInstances', 'conditionalVisValue' : '1'})

nodeTypeBuilder.setBuildOpChainFnc(buildInstancesCreateOpChain)
nodeTypeBuilder.build()
