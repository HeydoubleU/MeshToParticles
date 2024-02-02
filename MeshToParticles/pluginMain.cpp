#include <maya/MFnPlugin.h>
#include <maya/MGlobal.h>
#include <maya/MTypeId.h>
#include <maya/MObject.h>
#include <maya/MDataBlock.h>
#include <maya/MDataHandle.h>
#include <maya/MFnMesh.h>
#include <maya/MFnTypedAttribute.h>
#include <maya/MFnNumericAttribute.h>
#include <maya/MFnUnitAttribute.h>
#include <maya/MPxNode.h>
#include <maya/MPointArray.h>
#include <maya/MFnVectorArrayData.h>
#include <maya/MFnPointArrayData.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnParticleSystem.h>


void addScalarAttr(MFnDependencyNode& node, MString name) {
	MFnTypedAttribute attr;
	MObject attr_obj = attr.create(name, name, MFnData::kDoubleArray);
	attr.setStorable(false);
	attr.setWritable(true);
	attr.setReadable(true);
	node.addAttribute(attr_obj);
}


void addVectorAttr(MFnDependencyNode& node, MString name) {
	MFnTypedAttribute attr;
	MObject attr_obj = attr.create(name, name, MFnData::kVectorArray);
	attr.setStorable(false);
	attr.setWritable(true);
	attr.setReadable(true);
	node.addAttribute(attr_obj);
}


class MeshToParticles : public MPxNode
{
public:
	//MeshToParticles();
	virtual ~MeshToParticles() {};

	virtual MStatus compute(const MPlug& plug, MDataBlock& data);
	static void* creator();
	static MStatus initialize();
	int flip_flop = 0;

public:
	static MObject inMesh;
	static MObject createAttributes;
	static MObject output;
	static MTypeId id;
};
MTypeId MeshToParticles::id(0x00062C2B);
MObject MeshToParticles::inMesh;
MObject MeshToParticles::createAttributes;
MObject MeshToParticles::output;


void* MeshToParticles::creator() {
	return new MeshToParticles();
}

////////////////////////////////////////////////////////////////////////////////////////////////


MStatus MeshToParticles::initialize() {
	MFnTypedAttribute t_attr;
	MFnNumericAttribute n_attr;
	MFnUnitAttribute u_attr;

	// Create inMesh attribute as a mesh attribute
	inMesh = t_attr.create("inMesh", "im", MFnData::kMesh);
	t_attr.setStorable(false);
	t_attr.setWritable(true);
	t_attr.setReadable(true);
	addAttribute(inMesh);

	// Create createAttributes attribute as a bool attribute
	createAttributes = n_attr.create("createAttributes", "ca", MFnNumericData::kBoolean);
	n_attr.setStorable(true);
	n_attr.setWritable(true);
	n_attr.setReadable(true);
	addAttribute(createAttributes);
	
	// Create output attribute as a time attributet attribute as a time attribute
	output = u_attr.create("output", "o", MFnUnitAttribute::kTime);
	u_attr.setStorable(false);
	u_attr.setWritable(false);
	u_attr.setReadable(true);
	addAttribute(output);

	// Attribute affects
	attributeAffects(inMesh, output);
	attributeAffects(createAttributes, output);

	return MS::kSuccess;
}


MStatus MeshToParticles::compute(const MPlug& plug, MDataBlock& data)
{
	if (plug == output) {
		MPlug output_plug(thisMObject(), output);

		// get connected node
		MPlugArray connections;
		output_plug.connectedTo(connections, true, true);
		if (connections.length() == 0) {
			MGlobal::displayError("output must be connect to a particle node");
			return MS::kFailure;
		}

		else if (connections.length() > 1) {
			// print error message that this is not connected to anything
			MGlobal::displayWarning("Multiple connections, only the first will be evaluated. Output should be connected to a single particle node.");
		}

		// check output is connect to particle node
		if (!(connections[0].node().hasFn(MFn::kParticle))) {
			// print error message that this is connected to the wrong node type
			MGlobal::displayError("output must be connect to a particle node");
			return MS::kFailure;
		}

		// prep inputs
		MFnDependencyNode particle_fn(connections[0].node());
		MObject mesh_mobj = data.inputValue(inMesh).asMesh();
		MFnMesh mesh_fn(mesh_mobj);

		// set pos0
		MPointArray points;
		mesh_fn.getPoints(points, MSpace::kWorld);
		int count = points.length();
		MVectorArray pos(count);
		for (int i = 0; i < count; i++) {
			pos[i] = points[i];
		}
		MPlug pos0_plug = particle_fn.findPlug("pos0", false);
		pos0_plug.setMObject(MFnVectorArrayData().create(pos));

		// loop over color sets and set particle attributes
		MStringArray color_set_names;
		mesh_fn.getColorSetNames(color_set_names);
		for (MString& color_set_name : color_set_names)
		{
			bool is_scalar = mesh_fn.getColorRepresentation(color_set_name) == MFnMesh::kAlpha;
			MString name0 = color_set_name + "0"; // We always want to be setting the intial state version of the attribute

			if (!particle_fn.hasAttribute(name0)) {
				if (not data.inputValue(createAttributes).asBool()) {
					MGlobal::displayWarning("Attribute matching color set '" + name0 + "' does not exist");
					continue;
				}

				// create attribute on particle node
				if (is_scalar) {
					addScalarAttr(particle_fn, color_set_name);
					addScalarAttr(particle_fn, name0);
				}
				else {
					addVectorAttr(particle_fn, color_set_name);
					addVectorAttr(particle_fn, name0);
				}
			}

			MPlug dst_plug = particle_fn.findPlug(name0, false);
			MFnTypedAttribute typed_attr(dst_plug.attribute());

			// verify appropriate attribute exists on particle node
			if (is_scalar) {
				if (typed_attr.attrType() != MFnData::kDoubleArray) {
					MGlobal::displayWarning("Attribute/color set dtype mismatch. '" + color_set_name + "' requires attribute of type doubleArray.");
					continue;
				}
			} 
			else {
				if (typed_attr.attrType() != MFnData::kVectorArray) {
					MGlobal::displayWarning("Attribute/color set dtype mismatch. '" + color_set_name + "' requires attribute of type vectorArray.");
					continue;
				}
			}

			// color data
			MColorArray colors;
			mesh_fn.getVertexColors(colors, &color_set_name);
			count = colors.length();

			if (is_scalar) {
				// scalar attr (alpha only color set)
				MDoubleArray doubles(count, 1);
				for (int i = 0; i < count; i++) {
					doubles[i] = colors[i].a;
				}
				dst_plug.setMObject(MFnDoubleArrayData().create(doubles));

			}
			else {
				// vector attr (rgb or rgba color set, alpha is ignored)
				MVectorArray vectors(count);
				for (int i = 0; i < count; i++) {
					MColor c = colors[i];
					vectors[i] = MVector(c.r, c.g, c.b);
				}
				dst_plug.setMObject(MFnVectorArrayData().create(vectors));
			}
		}
		
		// decrement the particle's current time every compute to trigger particles update. Negative so we're before the default start frame which is 1 (ie. always in the initial state).
		// scuffed but whatever it works.
		flip_flop = (flip_flop-1) % -10;
		output_plug.setValue(flip_flop);
		data.setClean(plug);
		return MS::kSuccess;
	}
	else {
		return MS::kUnknownParameter;
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////


// Plugin initialization
MStatus initializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj, "Hyuu", "0.0");
	status = plugin.registerNode("meshToParticles", MeshToParticles::id, MeshToParticles::creator, MeshToParticles::initialize);
	MGlobal::displayInfo("MeshToParticles loaded yayeet");
	return MS::kSuccess;
}

MStatus uninitializePlugin(MObject obj)
{
	MStatus status;
	MFnPlugin plugin(obj);
	status = plugin.deregisterNode(MeshToParticles::id);
	MGlobal::displayInfo("MeshToParticles unloaded yeetya");
	return MS::kSuccess;
}