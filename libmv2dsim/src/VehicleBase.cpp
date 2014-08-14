/*+-------------------------------------------------------------------------+
  |                       MultiVehicle 2D simulator (libmv2dsim)            |
  |                                                                         |
  | Copyright (C) 2014  Jose Luis Blanco Claraco (University of Almeria)    |
  | Distributed under GNU General Public License version 3                  |
  |   See <http://www.gnu.org/licenses/>                                    |
  +-------------------------------------------------------------------------+  */

#include <mv2dsim/World.h>
#include <mv2dsim/VehicleBase.h>
#include <mv2dsim/VehicleDynamics/VehicleDifferential.h>

#include <rapidxml.hpp>
#include <rapidxml_utils.hpp>
#include <mrpt/utils/utils_defs.h>  // mrpt::format()

#include <sstream>      // std::stringstream

using namespace mv2dsim;
using namespace std;


// Protected ctor:
VehicleBase::VehicleBase(World *parent) : 
	VisualObject(parent),
	m_b2d_vehicle_body(NULL),
	m_q(0,0,0),
	m_dq(0,0,0)
{
}

void VehicleBase::simul_post_timestep_common(const TSimulContext &context)
{
	if (m_b2d_vehicle_body)
	{
		// Pos:
		const b2Vec2 &pos = m_b2d_vehicle_body->GetPosition();
		const float32 angle = m_b2d_vehicle_body->GetAngle();
		m_q.vals[0]=pos(0); 
		m_q.vals[1]=pos(1); 
		m_q.vals[2]=angle; 

		// Vel:
		const b2Vec2 &vel = m_b2d_vehicle_body->GetLinearVelocity();
		const float32 w = m_b2d_vehicle_body->GetAngularVelocity();
		m_dq.vals[0]=vel(0); 
		m_dq.vals[1]=vel(1); 
		m_dq.vals[2]=w; 
	}
}


/** Class factory: Creates a vehicle from XML description of type "<vehicle>...</vehicle>".  */
VehicleBase* VehicleBase::factory(World* parent, const rapidxml::xml_node<char> *root)
{
	using namespace std;
	using namespace rapidxml;

	if (!root) throw runtime_error("[VehicleBase::factory] XML node is NULL");
	if (0!=strcmp(root->name(),"vehicle")) throw runtime_error(mrpt::format("[VehicleBase::factory] XML root element is '%s' ('vehicle' expected)",root->name()));

	// Class factory according to: <dynamics class="XXX">
	// -------------------------------------------------
	VehicleBase *veh = NULL;
	const xml_node<> *dyn_node = root->first_node("dynamics");
	if (!dyn_node) throw runtime_error("[VehicleBase::factory] Missing XML node <dynamics>");

	const xml_attribute<> *dyn_class = dyn_node->first_attribute("class");
	if (!dyn_class || !dyn_class->value() ) throw runtime_error("[VehicleBase::factory] Missing mandatory attribute 'class' in node <dynamics>");

	if (!strcmp("differential",dyn_class->value()))
	{
		veh = new DynamicsDifferential(parent);
	}
	else 
	{
		throw runtime_error(mrpt::format("[VehicleBase::factory] Unknown vehicle dynamics class '%s'",dyn_class->value()));		
	}

	// Initialize here all common params shared by any polymorphic class:
	// -------------------------------------------------
	// attrib: name
		
	// (Mandatory) initial pose:
	{
		const xml_node<> *node = root->first_node("init_pose");
		if (!node) throw runtime_error("[VehicleBase::factory] Missing XML node <init_pose>");

		if (3!= ::sscanf(node->value(),"%lf %lf %lf",&veh->m_q.vals[0],&veh->m_q.vals[1],&veh->m_q.vals[2]))
			throw runtime_error("[VehicleBase::factory] Error parsing <init_pose>...</init_pose>");
		veh->m_q.vals[2] *= M_PI/180.0;
	}	
	
	// (Optional) initial vel:
	{
		const xml_node<> *node = root->first_node("init_vel");
		if (node)
		{
			if (3!= ::sscanf(node->value(),"%lf %lf %lf",&veh->m_dq.vals[0],&veh->m_dq.vals[1],&veh->m_dq.vals[2]))
				throw runtime_error("[VehicleBase::factory] Error parsing <init_vel>...</init_vel>");
			veh->m_dq.vals[2] *= M_PI/180.0;
		}
	}

	// Initialize class-specific params:
	// -------------------------------------------------
	veh->dynamics_load_params_from_xml(dyn_node);

	// Register bodies, fixtures, etc. in Box2D simulator:
	// ----------------------------------------------------
	b2World* b2world = parent->getBox2DWorld();
	veh->create_multibody_system(b2world);

	if (veh->m_b2d_vehicle_body)
	{
		// Init pos:
		veh->m_b2d_vehicle_body->SetTransform( b2Vec2( veh->m_q.vals[0], veh->m_q.vals[1] ),  veh->m_q.vals[2] );
		// Init vel:
		veh->m_b2d_vehicle_body->SetLinearVelocity( b2Vec2(veh->m_dq.vals[0], veh->m_dq.vals[1] ) );
		veh->m_b2d_vehicle_body->SetAngularVelocity(veh->m_dq.vals[2] );
	}

	// Vehicle controller:
	// -------------------------------------------------

	// Friction model:
	// -------------------------------------------------

	// Sensors:
	// -------------------------------------------------


	return veh;
}

VehicleBase* VehicleBase::factory(World* parent, const std::string &xml_text)
{
	// Parse the string as if it was an XML file:
	std::stringstream s;
	s.str( xml_text );

	rapidxml::file<> fil(s);
	char* input_str = const_cast<char*>(xml_text.c_str());
	rapidxml::xml_document<> xml;
	try {
		xml.parse<0>(input_str);
	}
	catch (rapidxml::parse_error &e) {
		unsigned int line = static_cast<long>(std::count(input_str, e.where<char>(), '\n') + 1);
		throw std::runtime_error( mrpt::format("[VehicleBase::factory] XML parse error (Line %u): %s", static_cast<unsigned>(line), line, e.what() ) );
	}
	return VehicleBase::factory(parent,xml.first_node());
}

/** Loads vehicle params from input XML node of type "<vehicle>...</vehicle>". 
	* See derived classes & documentation for a list of accepted params.  
	*/
void VehicleBase::load_params_from_xml(const rapidxml::xml_node<char> *xml_node)
{
}

/// \overload
void VehicleBase::load_params_from_xml(const std::string &xml_text)
{

}
