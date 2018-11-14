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
	generateTypeParam = node.getParameter('generateType')
	fileTypeParam = node.getParameter('fileType')
	filePathParam = node.getParameter('filePath')
	numPointsParam = node.getParameter('numPoints')
	splitPointcloudLocationsParam = node.getParameter('splitPointcloudLocations')
	pointWidthTypeParam = node.getParameter('pointWidthType')
	constantPointWidthParam = node.getParameter('constantPointWidth')
	randomPointWidthMinParam = node.getParameter('randomPointWidthMin')
	randomPointWidthMaxParam = node.getParameter('randomPointWidthMax')
	areaSpreadParamX = node.getParameter('areaSpread.i0')
	areaSpreadParamY = node.getParameter('areaSpread.i1')
	areaSpreadParamZ = node.getParameter('areaSpread.i2')
	extraFloatPrimvarTypeParam = node.getParameter('extraFloatPrimvarType')
	extraVectorPrimvarTypeParam = node.getParameter('extraVectorPrimvarType')
	extraColorPrimvarTypeParam = node.getParameter('extraColorPrimvarType')
	if targetLocationParam:
		location = targetLocationParam.getValue(0)

		locationPaths = location[1:].split('/')[1:]
		attrsHierarchy = 'c.' + '.c.'.join(locationPaths)

		argsGb.set(attrsHierarchy + '.a.generateType', FnAttribute.IntAttribute(generateTypeParam.getValue(0)))
		if generateTypeParam.getValue(0) == 1:
			argsGb.set(attrsHierarchy + '.a.fileType', FnAttribute.IntAttribute(fileTypeParam.getValue(0)))
			argsGb.set(attrsHierarchy + '.a.filePath', FnAttribute.StringAttribute(filePathParam.getValue(0)))
		else:
			argsGb.set(attrsHierarchy + '.a.numPoints', FnAttribute.IntAttribute(numPointsParam.getValue(0)))
			argsGb.set(attrsHierarchy + '.a.splitPointcloudLocations', FnAttribute.IntAttribute(splitPointcloudLocationsParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.pointWidthType', FnAttribute.IntAttribute(pointWidthTypeParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.constantPointWidth', FnAttribute.FloatAttribute(constantPointWidthParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.randomPointWidthMin', FnAttribute.FloatAttribute(randomPointWidthMinParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.randomPointWidthMax', FnAttribute.FloatAttribute(randomPointWidthMaxParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.areaSpread', FnAttribute.FloatAttribute([areaSpreadParamX.getValue(0), areaSpreadParamY.getValue(0), areaSpreadParamZ.getValue(0)], 3))
		argsGb.set(attrsHierarchy + '.a.extraFloatPrimvarType', FnAttribute.IntAttribute(extraFloatPrimvarTypeParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.extraVectorPrimvarType', FnAttribute.IntAttribute(extraVectorPrimvarTypeParam.getValue(0)))
		argsGb.set(attrsHierarchy + '.a.extraColorPrimvarType', FnAttribute.IntAttribute(extraColorPrimvarTypeParam.getValue(0)))

	interface.appendOp('PointCloudCreate', argsGb.build())

nodeTypeBuilder = Nodes3DAPI.NodeTypeBuilder('PointCloudCreate')

gb = FnAttribute.GroupBuilder()
gb.set('location', FnAttribute.StringAttribute('/root/world/geo/pointcloud1'))
gb.set('generateType', FnAttribute.IntAttribute(0))
gb.set('fileType', FnAttribute.IntAttribute(0))
gb.set('filePath', FnAttribute.StringAttribute(""))
gb.set('numPoints', FnAttribute.IntAttribute(10000))
gb.set('splitPointcloudLocations', FnAttribute.IntAttribute(0))
gb.set('pointWidthType', FnAttribute.IntAttribute(0))
gb.set('constantPointWidth', FnAttribute.FloatAttribute(0.1))
gb.set('randomPointWidthMin', FnAttribute.FloatAttribute(0.1))
gb.set('randomPointWidthMax', FnAttribute.FloatAttribute(0.2))
gb.set('areaSpread', FnAttribute.FloatAttribute([20.0, 20.0, 20.0], 3))
gb.set('extraFloatPrimvarType', FnAttribute.IntAttribute(0))
gb.set('extraVectorPrimvarType', FnAttribute.IntAttribute(0))
gb.set('extraColorPrimvarType', FnAttribute.IntAttribute(0))

nodeTypeBuilder.setParametersTemplateAttr(gb.build())

nodeTypeBuilder.setHintsForParameter('location', {'widget' : 'scenegraphLocation'})
nodeTypeBuilder.setHintsForParameter('generateType', {'widget' : 'mapper',
				 'options' : 'regular cube of points:0|read from file:1'})
nodeTypeBuilder.setHintsForParameter('fileType', {'widget' : 'mapper',
				 'options' : 'ASCII positions (space separated):0|ASCII positions and radii (space separated):1',
				 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '1'})
nodeTypeBuilder.setHintsForParameter('filePath', {'widget' : 'assetIdInput',
				 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '1'})
nodeTypeBuilder.setHintsForParameter('numPoints', {'int' : True, 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('splitPointcloudLocations', {'widget' : 'checkBox', 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('pointWidthType', {'widget' : 'mapper',
				 'options' : 'single constant width:0|per point constant width:1|random width per point:2'})
nodeTypeBuilder.setHintsForParameter('constantPointWidth', {'conditionalVisOp' : 'lessThanOrEqualTo', 'conditionalVisPath' : '../pointWidthType', 'conditionalVisValue' : '1'})
nodeTypeBuilder.setHintsForParameter('randomPointWidthMin', {'conditionalVisOp' : 'greaterThan', 'conditionalVisPath' : '../pointWidthType', 'conditionalVisValue' : '1'})
nodeTypeBuilder.setHintsForParameter('randomPointWidthMax', {'conditionalVisOp' : 'greaterThan', 'conditionalVisPath' : '../pointWidthType', 'conditionalVisValue' : '1'})
nodeTypeBuilder.setHintsForParameter('areaSpread', {'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('extraFloatPrimvarType', {'widget' : 'mapper',
				 'options' : 'off:0|per point constant value:1|random value per point:2',
				 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('extraVectorPrimvarType', {'widget' : 'mapper',
				 'options' : 'off:0|per point constant value:1|random value per point:2',
				 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '0'})
nodeTypeBuilder.setHintsForParameter('extraColorPrimvarType', {'widget' : 'mapper',
				 'options' : 'off:0|per point constant value:1|random value per point:2',
				 'conditionalVisOp' : 'equalTo', 'conditionalVisPath' : '../generateType', 'conditionalVisValue' : '0'})

nodeTypeBuilder.setBuildOpChainFnc(buildPointCloudCreateOpChain)
nodeTypeBuilder.build()
