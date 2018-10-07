'''
 PointCloudCreate
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

def buildPointCloudCreateOpChain(node, interface):
	interface.setMinRequiredInputs(0)
	argsGb = FnAttribute.GroupBuilder()

	targetLocationParam = node.getParameter('location')
	numPointsParam = node.getParameter('numPoints')
	pointWidthParam = node.getParameter('pointWidth')
	areaSpreadParamX = node.getParameter('areaSpread.i0')
	areaSpreadParamY = node.getParameter('areaSpread.i1')
	areaSpreadParamZ = node.getParameter('areaSpread.i2')
	if targetLocationParam:
		location = targetLocationParam.getValue(0)

		locationPaths = location[1:].split('/')[1:]
		attrsHierarchy = 'c.' + '.c.'.join(locationPaths)

		argsGb.set(attrsHierarchy + '.a.numPoints', FnAttribute.IntAttribute(numPointsParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.pointWidth', FnAttribute.FloatAttribute(pointWidthParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.areaSpread', FnAttribute.FloatAttribute([areaSpreadParamX.getValue(0), areaSpreadParamY.getValue(0), areaSpreadParamZ.getValue(0)], 3))

	interface.appendOp('PointCloudCreate', argsGb.build())

nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('PointCloudCreate')

gb = FnAttribute.GroupBuilder()
gb.set('location', FnAttribute.StringAttribute('/root/world/geo/pointcloud1'))
gb.set('numPoints', FnAttribute.IntAttribute(10000))
gb.set('pointWidth', FnAttribute.FloatAttribute(0.1))
gb.set('areaSpread', FnAttribute.FloatAttribute([20.0, 20.0, 20.0], 3))

nodeTypeBuilder.setParametersTemplateAttr(gb.build())

nodeTypeBuilder.setHintsForParameter('location', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('numPoints', {'int' : True})

nodeTypeBuilder.setBuildOpChainFnc(buildPointCloudCreateOpChain)
nodeTypeBuilder.build()
