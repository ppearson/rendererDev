'''
 Instancer
 Created by Peter Pearson in 2016-2018.

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

def buildInstancerOpChain(node, interface):
	interface.setMinRequiredInputs(0)
	argsGb = FnAttribute.GroupBuilder()

	targetLocationParam = node.getParameter('targetLocation')
	sourceLocationParam = node.getParameter('sourceLocation')
	numInstancesParam = node.getParameter('numInstances')
	instanceArrayParam = node.getParameter('instanceArray')
	threeDParam = node.getParameter('threeD')
	areaSpreadParam = node.getParameter('areaSpread')
	groupInstancesParam = node.getParameter('groupInstances')
	groupSizeParam = node.getParameter('groupSize')
	if targetLocationParam:
		location = targetLocationParam.getValue(0)

		locationPaths = location[1:].split('/')[1:]
		attrsHierarchy = 'c.' + '.c.'.join(locationPaths)

		argsGb.set(attrsHierarchy + '.a.sourceLocation', FnAttribute.StringAttribute(sourceLocationParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.numInstances', FnAttribute.IntAttribute(numInstancesParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.instanceArray', FnAttribute.IntAttribute(instanceArrayParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.threeD', FnAttribute.IntAttribute(threeDParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.areaSpread', FnAttribute.FloatAttribute(areaSpreadParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.groupInstances', FnAttribute.IntAttribute(groupInstancesParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.groupSize', FnAttribute.IntAttribute(groupSizeParam.getValue(0)))

	interface.appendOp('Instancer', argsGb.build())

nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('Instancer')

gb = FnAttribute.GroupBuilder()
gb.set('targetLocation', FnAttribute.StringAttribute('/root/world/geo/instancer1'))
gb.set('sourceLocation', FnAttribute.StringAttribute(''))
gb.set('numInstances', FnAttribute.IntAttribute(100))
gb.set('instanceArray', FnAttribute.IntAttribute(0))
gb.set('threeD', FnAttribute.IntAttribute(0))
gb.set('areaSpread', FnAttribute.FloatAttribute(30.0))
gb.set('groupInstances', FnAttribute.IntAttribute(0))
gb.set('groupSize', FnAttribute.IntAttribute(400))

nodeTypeBuilder.setParametersTemplateAttr(gb.build())

nodeTypeBuilder.setHintsForParameter('targetLocation', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('sourceLocation', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('instanceArray', {'widget' : 'checkBox'})
nodeTypeBuilder.setHintsForParameter('threeD', {'widget' : 'checkBox'})
nodeTypeBuilder.setHintsForParameter('numInstances', {'int' : True})
nodeTypeBuilder.setHintsForParameter('groupInstances', {'widget' : 'checkBox', 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../instanceArray', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('groupSize', {'int' : True, 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../groupInstances', 'conditionalVisValue' : '1'})

nodeTypeBuilder.setBuildOpChainFnc(buildInstancerOpChain)
nodeTypeBuilder.build()
