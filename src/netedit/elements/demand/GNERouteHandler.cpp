/****************************************************************************/
// Eclipse SUMO, Simulation of Urban MObility; see https://eclipse.org/sumo
// Copyright (C) 2001-2021 German Aerospace Center (DLR) and others.
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// https://www.eclipse.org/legal/epl-2.0/
// This Source Code may also be made available under the following Secondary
// Licenses when the conditions for such availability set forth in the Eclipse
// Public License 2.0 are satisfied: GNU General Public License, version 2
// or later which is available at
// https://www.gnu.org/licenses/old-licenses/gpl-2.0-standalone.html
// SPDX-License-Identifier: EPL-2.0 OR GPL-2.0-or-later
/****************************************************************************/
/// @file    GNERouteHandler.cpp
/// @author  Pablo Alvarez Lopez
/// @date    Jan 2019
///
// Builds demand objects for netedit
/****************************************************************************/
#include <config.h>
#include <netedit/GNENet.h>
#include <netedit/GNEUndoList.h>
#include <netedit/GNEViewNet.h>
#include <netedit/changes/GNEChange_DemandElement.h>

#include "GNEContainer.h"
#include "GNEStopContainer.h"
#include "GNEPerson.h"
#include "GNEStopPerson.h"
#include "GNEPersonTrip.h"
#include "GNERide.h"
#include "GNERoute.h"
#include "GNERouteHandler.h"
#include "GNEStop.h"
#include "GNETranship.h"
#include "GNETransport.h"
#include "GNEVehicle.h"
#include "GNEVehicleType.h"
#include "GNEWalk.h"


// ===========================================================================
// member method definitions
// ===========================================================================

GNERouteHandler::GNERouteHandler(const std::string& file, GNENet* net, bool undoDemandElements) :
    RouteHandler(file),
    myNet(net),
    myUndoDemandElements(undoDemandElements) {
}


GNERouteHandler::~GNERouteHandler() {}


void 
GNERouteHandler::buildVType(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVTypeParameter& vTypeParameter,
    const std::map<std::string, std::string> &parameters) {
    // first check if we're creating a vType or a pType
    SumoXMLTag vTypeTag = (vTypeParameter.vehicleClass == SVC_PEDESTRIAN) ? SUMO_TAG_PTYPE : SUMO_TAG_VTYPE;
    // check if loaded vType/pType is a default vtype
    if ((vTypeParameter.id == DEFAULT_VTYPE_ID) || (vTypeParameter.id == DEFAULT_PEDTYPE_ID) || (vTypeParameter.id == DEFAULT_BIKETYPE_ID)) {
        // overwrite default vehicle type
        GNEVehicleType::overwriteVType(myNet->retrieveDemandElement(vTypeTag, vTypeParameter.id, false), vTypeParameter, myNet->getViewNet()->getUndoList());
    } else if (myNet->retrieveDemandElement(vTypeTag, vTypeParameter.id, false) != nullptr) {
        WRITE_ERROR("There is another " + toString(vTypeTag) + " with the same ID='" + vTypeParameter.id + "'.");
    } else {
        // create vType/pType using myCurrentVType
        GNEDemandElement* vType = new GNEVehicleType(myNet, vTypeParameter, vTypeTag);
        if (myUndoDemandElements) {
            myNet->getViewNet()->getUndoList()->p_begin("add " + vType->getTagStr());
            myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(vType, true), true);
            myNet->getViewNet()->getUndoList()->p_end();
        } else {
            myNet->getAttributeCarriers()->insertDemandElement(vType);
            vType->incRef("buildVType");
        }
    }
}


void 
GNERouteHandler::buildVTypeDistribution(const CommonXMLStructure::SumoBaseObject* /*sumoBaseObject*/, const std::string &/*id*/) {
    // unsuported
    WRITE_ERROR("NETEDIT doesn't support vType distributions");
}


void 
GNERouteHandler::buildRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &id, SUMOVehicleClass vClass, 
    const std::vector<std::string> &edgeIDs, const RGBColor &color, const int repeat, const SUMOTime cycleTime, 
    const std::map<std::string, std::string> &parameters) {
    // parse edges
    const auto edges = parseEdges(SUMO_TAG_ROUTE, edgeIDs);
    // create GNERoute
    GNEDemandElement* route = new GNERoute(myNet, id, vClass, edges, color, repeat, cycleTime, parameters);
    if (myUndoDemandElements) {
        myNet->getViewNet()->getUndoList()->p_begin("add " + route->getTagStr());
        myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(route, true), true);
/*
        // iterate over stops of myActiveRouteStops and create stops associated with it
        for (const auto& i : activeStops) {
            buildStop(net, true, i, route);
        }
*/
        myNet->getViewNet()->getUndoList()->p_end();
    } else {
        myNet->getAttributeCarriers()->insertDemandElement(route);
        for (const auto& edge : edges) {
            edge->addChildElement(route);
        }
        route->incRef("buildRoute");
/*
        // iterate over stops of myActiveRouteStops and create stops associated with it
        for (const auto& i : activeStops) {
            buildStop(net, false, i, route);
        }
*/
    }
}


void 
GNERouteHandler::buildRouteDistribution(const CommonXMLStructure::SumoBaseObject* /*sumoBaseObject*/, const std::string &/*id*/) {
    // unsuported
    WRITE_ERROR("NETEDIT doesn't support route distributions");
}


void 
GNERouteHandler::buildVehicleOverRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters,
    const std::map<std::string, std::string> &parameters) {
    // first check if ID is duplicated
    if (!isVehicleIdDuplicated(myNet, vehicleParameters.id)) {
        // obtain routes and vtypes
        GNEDemandElement* vType = myNet->retrieveDemandElement(SUMO_TAG_VTYPE, vehicleParameters.vtypeid, false);
        GNEDemandElement* route = myNet->retrieveDemandElement(SUMO_TAG_ROUTE, vehicleParameters.routeid, false);
        if (vType == nullptr) {
            WRITE_ERROR("Invalid vehicle type '" + vehicleParameters.vtypeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (route == nullptr) {
            WRITE_ERROR("Invalid route '" + vehicleParameters.routeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTLANE_SET) && (vehicleParameters.departLaneProcedure == DepartLaneDefinition::GIVEN) && ((int)route->getParentEdges().front()->getLanes().size() < vehicleParameters.departLane)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTLANE) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departLane) + " is greater than number of lanes");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTSPEED_SET) && (vehicleParameters.departSpeedProcedure == DepartSpeedDefinition::GIVEN) && (vType->getAttributeDouble(SUMO_ATTR_MAXSPEED) < vehicleParameters.departSpeed)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTSPEED) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departSpeed) + " is greater than vType" + toString(SUMO_ATTR_MAXSPEED));
        } else {
            // create vehicle using vehicleParameters
            GNEDemandElement* vehicle = new GNEVehicle(myNet, vType, route, vehicleParameters);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + vehicle->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(vehicle, true), true);
                // iterate over stops of vehicleParameters and create stops associated with it
/*
                for (const auto& i : vehicleParameters.stops) {
                    buildStop(myNet, true, i, vehicle);
                }
*/
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(vehicle);
                // set vehicle as child of vType and Route
                vType->addChildElement(vehicle);
                route->addChildElement(vehicle);
                vehicle->incRef("buildVehicleOverRoute");
/*
                // iterate over stops of vehicleParameters and create stops associated with it
                for (const auto& stop : vehicleParameters.stops) {
                    buildStop(myNet, false, stop, vehicle);
                }
*/
            }
            // center view after creation
            if (!myNet->getViewNet()->getVisibleBoundary().around(vehicle->getPositionInView())) {
                myNet->getViewNet()->centerTo(vehicle->getPositionInView(), false);
            }
        }
    }
}


void 
GNERouteHandler::buildFlowOverRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters,
    const std::map<std::string, std::string> &parameters) {
    // first check if ID is duplicated
    if (!isVehicleIdDuplicated(myNet, vehicleParameters.id)) {
        // obtain routes and vtypes
        GNEDemandElement* vType = myNet->retrieveDemandElement(SUMO_TAG_VTYPE, vehicleParameters.vtypeid, false);
        GNEDemandElement* route = myNet->retrieveDemandElement(SUMO_TAG_ROUTE, vehicleParameters.routeid, false);
        if (vType == nullptr) {
            WRITE_ERROR("Invalid vehicle type '" + vehicleParameters.vtypeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (route == nullptr) {
            WRITE_ERROR("Invalid route '" + vehicleParameters.routeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTLANE_SET) && (vehicleParameters.departLaneProcedure == DepartLaneDefinition::GIVEN) && ((int)route->getParentEdges().front()->getLanes().size() < vehicleParameters.departLane)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTLANE) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departLane) + " is greater than number of lanes");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTSPEED_SET) && (vehicleParameters.departSpeedProcedure == DepartSpeedDefinition::GIVEN) && (vType->getAttributeDouble(SUMO_ATTR_MAXSPEED) < vehicleParameters.departSpeed)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTSPEED) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departSpeed) + " is greater than vType" + toString(SUMO_ATTR_MAXSPEED));
        } else {
            // create flow or trips using vehicleParameters
            GNEDemandElement* flow = new GNEVehicle(myNet, vType, route, vehicleParameters);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + flow->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(flow, true), true);
                // iterate over stops of vehicleParameters and create stops associated with it
/*
                for (const auto& i : vehicleParameters.stops) {
                    buildStop(myNet, true, i, flow);
                }
*/
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(flow);
                // set flow as child of vType and Route
                vType->addChildElement(flow);
                route->addChildElement(flow);
                flow->incRef("buildFlowOverRoute");
                // iterate over stops of vehicleParameters and create stops associated with it
/*
                for (const auto& i : vehicleParameters.stops) {
                    buildStop(myNet, false, i, flow);
                }
*/
            }
            // center view after creation
            if (!myNet->getViewNet()->getVisibleBoundary().around(flow->getPositionInView())) {
                myNet->getViewNet()->centerTo(flow->getPositionInView(), false);
            }
        }
    }
}


void
GNERouteHandler::buildVehicleEmbeddedRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, SUMOVehicleParameter vehicleParameters, 
    const std::vector<std::string>& edgeIDs, const std::map<std::string, std::string> &parameters) {
    // parse edges
    const auto edges = parseEdges(SUMO_TAG_ROUTE, edgeIDs);
    // first check if ID is duplicated
    if (!isVehicleIdDuplicated(myNet, vehicleParameters.id) && (edges.size() > 0)) {
        // obtain vType
        GNEDemandElement* vType = myNet->retrieveDemandElement(SUMO_TAG_VTYPE, vehicleParameters.vtypeid, false);
        if (vType == nullptr) {
            WRITE_ERROR("Invalid vehicle type '" + vehicleParameters.vtypeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTLANE_SET) && (vehicleParameters.departLaneProcedure == DepartLaneDefinition::GIVEN) && ((int)edges.front()->getLanes().size() < vehicleParameters.departLane)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTLANE) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departLane) + " is greater than number of lanes");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTSPEED_SET) && (vehicleParameters.departSpeedProcedure == DepartSpeedDefinition::GIVEN) && (vType->getAttributeDouble(SUMO_ATTR_MAXSPEED) < vehicleParameters.departSpeed)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTSPEED) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departSpeed) + " is greater than vType" + toString(SUMO_ATTR_MAXSPEED));
        } else {
            // due vehicle was loaded without a route, change tag
            vehicleParameters.tag = GNE_TAG_VEHICLE_WITHROUTE;
            // create vehicle or trips using myTemporalVehicleParameter without a route
            GNEDemandElement* vehicle = new GNEVehicle(myNet, vType, vehicleParameters);
            // creaste embedded route
            GNEDemandElement* embeddedRoute = new GNERoute(myNet, vehicle, edges, RGBColor::CYAN, 0, 0, std::map<std::string, std::string>());
            // add both to net depending of myUndoDemandElements
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add vehicle and " + embeddedRoute->getTagStr());
                // add both in net using undoList
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(vehicle, true), true);
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(embeddedRoute, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                // add vehicleOrRouteFlow in net and in their vehicle type parent
                myNet->getAttributeCarriers()->insertDemandElement(vehicle);
                // set vehicle as child of vType
                vType->addChildElement(vehicle);
                vehicle->incRef("buildVehicleWithEmbeddedRoute");
                // add route manually in net, and in all of their edges and in vehicleOrRouteFlow
                myNet->getAttributeCarriers()->insertDemandElement(embeddedRoute);
                for (const auto& edge : edges) {
                    edge->addChildElement(vehicle);
                }
                // set route as child of vehicle
                vehicle->addChildElement(embeddedRoute);
                embeddedRoute->incRef("buildVehicleWithEmbeddedRoute");
            }
            // center view after creation
            if (!myNet->getViewNet()->getVisibleBoundary().around(vehicle->getPositionInView())) {
                myNet->getViewNet()->centerTo(vehicle->getPositionInView(), false);
            }
        }
    }
}


void 
GNERouteHandler::buildFlowEmbeddedRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, SUMOVehicleParameter vehicleParameters, 
    const std::vector<std::string>& edgeIDs, const std::map<std::string, std::string> &parameters) {
    // parse edges
    const auto edges = parseEdges(SUMO_TAG_ROUTE, edgeIDs);
    // first check if ID is duplicated
    if (!isVehicleIdDuplicated(myNet, vehicleParameters.id) && edges.size() > 0) {
        // obtain vType
        GNEDemandElement* vType = myNet->retrieveDemandElement(SUMO_TAG_VTYPE, vehicleParameters.vtypeid, false);
        if (vType == nullptr) {
            WRITE_ERROR("Invalid vehicle type '" + vehicleParameters.vtypeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTLANE_SET) && (vehicleParameters.departLaneProcedure == DepartLaneDefinition::GIVEN) && ((int)edges.front()->getLanes().size() < vehicleParameters.departLane)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTLANE) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departLane) + " is greater than number of lanes");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTSPEED_SET) && (vehicleParameters.departSpeedProcedure == DepartSpeedDefinition::GIVEN) && (vType->getAttributeDouble(SUMO_ATTR_MAXSPEED) < vehicleParameters.departSpeed)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTSPEED) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departSpeed) + " is greater than vType" + toString(SUMO_ATTR_MAXSPEED));
        } else {
            // due vehicle was loaded without a route, change tag
            vehicleParameters.tag = GNE_TAG_FLOW_WITHROUTE;
            // create vehicle or trips using myTemporalVehicleParameter without a route
            GNEDemandElement* flow = new GNEVehicle(myNet, vType, vehicleParameters);
            // creaste embedded route
            GNEDemandElement* embeddedRoute = new GNERoute(myNet, flow, edges, RGBColor::CYAN, 0, 0, std::map<std::string, std::string>());
            // add both to net depending of myUndoDemandElements
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add vehicle and " + embeddedRoute->getTagStr());
                // add both in net using undoList
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(flow, true), true);
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(embeddedRoute, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                // add vehicleOrRouteFlow in net and in their vehicle type parent
                myNet->getAttributeCarriers()->insertDemandElement(flow);
                // set vehicle as child of vType
                vType->addChildElement(flow);
                flow->incRef("buildFlowWithEmbeddedRoute");
                // add route manually in net, and in all of their edges and in vehicleOrRouteFlow
                myNet->getAttributeCarriers()->insertDemandElement(embeddedRoute);
                for (const auto& edge : edges) {
                    edge->addChildElement(flow);
                }
                // set route as child of flow
                flow->addChildElement(embeddedRoute);
                embeddedRoute->incRef("buildFlowWithEmbeddedRoute");
            }
            // center view after creation
            if (!myNet->getViewNet()->getVisibleBoundary().around(flow->getPositionInView())) {
                myNet->getViewNet()->centerTo(flow->getPositionInView(), false);
            }
        }
    }
}


void
GNERouteHandler::buildTrip(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters, 
    const std::string &fromEdgeID, const std::string &toEdgeID, const std::vector<std::string>& viaIDs, const std::map<std::string, std::string> &parameters) {
    // parse edges
    const auto fromEdge = parseEdge(SUMO_TAG_TRIP, fromEdgeID);
    const auto toEdge = parseEdge(SUMO_TAG_TRIP, toEdgeID);
    const auto via = parseEdges(SUMO_TAG_TRIP, viaIDs);
    // check if exist another vehicle with the same ID (note: Vehicles, Flows and Trips share namespace)
    if (fromEdge && toEdge && !isVehicleIdDuplicated(myNet, vehicleParameters.id)) {
        // obtain  vtypes
        GNEDemandElement* vType = myNet->retrieveDemandElement(SUMO_TAG_VTYPE, vehicleParameters.vtypeid, false);
        if (vType == nullptr) {
            WRITE_ERROR("Invalid vehicle type '" + vehicleParameters.vtypeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTLANE_SET) && ((vehicleParameters.departLaneProcedure == DepartLaneDefinition::GIVEN)) && ((int)fromEdge->getLanes().size() < vehicleParameters.departLane)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTLANE) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departLane) + " is greater than number of lanes");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTSPEED_SET) && (vehicleParameters.departSpeedProcedure == DepartSpeedDefinition::GIVEN) && (vType->getAttributeDouble(SUMO_ATTR_MAXSPEED) < vehicleParameters.departSpeed)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTSPEED) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departSpeed) + " is greater than vType" + toString(SUMO_ATTR_MAXSPEED));
        } else {
            // add "via" edges in vehicleParameters
            for (const auto& viaEdge : via) {
                vehicleParameters.via.push_back(viaEdge->getID());
            }
            // create trip or flow using tripParameters
            GNEDemandElement* trip = new GNEVehicle(myNet, vType, fromEdge, toEdge, via, vehicleParameters);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + trip->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(trip, true), true);
                // iterate over stops of vehicleParameters and create stops associated with it
/*
                for (const auto& stop : vehicleParameters.stops) {
                    buildStop(myNet, true, stop, trip);
                }
*/
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(trip);
                // set vehicle as child of vType
                vType->addChildElement(trip);
                trip->incRef("buildTrip");
                // add reference in all edges
                fromEdge->addChildElement(trip);
                toEdge->addChildElement(trip);
                for (const auto& viaEdge : via) {
                    viaEdge->addChildElement(trip);
                }
/*
                // iterate over stops of vehicleParameters and create stops associated with it
                for (const auto& stop : vehicleParameters.stops) {
                    buildStop(myNet, false, stop, trip);
                }
*/
            }
            // center view after creation
            if (!myNet->getViewNet()->getVisibleBoundary().around(trip->getPositionInView())) {
                myNet->getViewNet()->centerTo(trip->getPositionInView(), false);
            }
        }
    }
}


void 
GNERouteHandler::buildFlow(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters, 
    const std::string &fromEdgeID, const std::string &toEdgeID, const std::vector<std::string>& viaIDs, const std::map<std::string, std::string> &parameters) {
    // parse edges
    const auto fromEdge = parseEdge(SUMO_TAG_TRIP, fromEdgeID);
    const auto toEdge = parseEdge(SUMO_TAG_TRIP, toEdgeID);
    const auto via = parseEdges(SUMO_TAG_TRIP, viaIDs);
    // check if exist another vehicle with the same ID (note: Vehicles, Flows and Trips share namespace)
    if (fromEdge && toEdge && !isVehicleIdDuplicated(myNet, vehicleParameters.id)) {
        // obtain  vtypes
        GNEDemandElement* vType = myNet->retrieveDemandElement(SUMO_TAG_VTYPE, vehicleParameters.vtypeid, false);
        if (vType == nullptr) {
            WRITE_ERROR("Invalid vehicle type '" + vehicleParameters.vtypeid + "' used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'.");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTLANE_SET) && (vehicleParameters.departLaneProcedure == DepartLaneDefinition::GIVEN) && ((int)fromEdge->getLanes().size() < vehicleParameters.departLane)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTLANE) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departLane) + " is greater than number of lanes");
        } else if (vehicleParameters.wasSet(VEHPARS_DEPARTSPEED_SET) && (vehicleParameters.departSpeedProcedure == DepartSpeedDefinition::GIVEN) && (vType->getAttributeDouble(SUMO_ATTR_MAXSPEED) < vehicleParameters.departSpeed)) {
            WRITE_ERROR("Invalid " + toString(SUMO_ATTR_DEPARTSPEED) + " used in " + toString(vehicleParameters.tag) + " '" + vehicleParameters.id + "'. " + toString(vehicleParameters.departSpeed) + " is greater than vType" + toString(SUMO_ATTR_MAXSPEED));
        } else {
            // add "via" edges in vehicleParameters
            for (const auto& viaEdge : via) {
                vehicleParameters.via.push_back(viaEdge->getID());
            }
            // create trip or flow using tripParameters
            GNEDemandElement* flow = new GNEVehicle(myNet, vType, fromEdge, toEdge, via, vehicleParameters);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + flow->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(flow, true), true);
                // iterate over stops of vehicleParameters and create stops associated with it
/*
                for (const auto& stop : vehicleParameters.stops) {
                    buildStop(myNet, true, stop, flow);
                }
*/
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(flow);
                // set vehicle as child of vType
                vType->addChildElement(flow);
                flow->incRef("buildFlow");
                // add reference in all edges
                fromEdge->addChildElement(flow);
                toEdge->addChildElement(flow);
                for (const auto& viaEdge : via) {
                    viaEdge->addChildElement(flow);
                }
/*
                // iterate over stops of vehicleParameters and create stops associated with it
                for (const auto& stop : vehicleParameters.stops) {
                    buildStop(myNet, false, stop, flow);
                }
*/
            }
            // center view after creation
            if (!myNet->getViewNet()->getVisibleBoundary().around(flow->getPositionInView())) {
                myNet->getViewNet()->centerTo(flow->getPositionInView(), false);
            }
        }
    }
}


void
GNERouteHandler::buildStop(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter::Stop& stopParameters) {
    // get stop parent
    GNEDemandElement *stopParent = myNet->retrieveDemandElement(sumoBaseObject->getParentSumoBaseObject()->getTag(), sumoBaseObject->getStringAttribute(SUMO_ATTR_ID));
    // declare pointers to parent elements
    GNEAdditional* stoppingPlace = nullptr;
    GNELane* lane = nullptr;
    // GNEEdge* edge = nullptr;
    SumoXMLTag stopTagType = SUMO_TAG_NOTHING;
    bool validParentDemandElement = true;
    if (stopParameters.busstop.size() > 0) {
        stoppingPlace = myNet->retrieveAdditional(SUMO_TAG_BUS_STOP, stopParameters.busstop, false);
        // distinguish between stop for vehicles and stops for persons
        if (stopParent->getTagProperty().isPerson()) {
            stopTagType = GNE_TAG_STOPPERSON_BUSSTOP;
        } else {
            stopTagType = SUMO_TAG_STOP_BUSSTOP;
        }
    } else if (stopParameters.containerstop.size() > 0) {
        stoppingPlace = myNet->retrieveAdditional(SUMO_TAG_CONTAINER_STOP, stopParameters.containerstop, false);
        // distinguish between stop for vehicles and stops for persons
        if (stopParent->getTagProperty().isPerson()) {
            WRITE_ERROR("Persons don't support stops at container stops");
            validParentDemandElement = false;
        } else {
            stopTagType = SUMO_TAG_STOP_CONTAINERSTOP;
        }
    } else if (stopParameters.chargingStation.size() > 0) {
        stoppingPlace = myNet->retrieveAdditional(SUMO_TAG_CHARGING_STATION, stopParameters.chargingStation, false);
        // distinguish between stop for vehicles and stops for persons
        if (stopParent->getTagProperty().isPerson()) {
            WRITE_ERROR("Persons don't support stops at charging stations");
            validParentDemandElement = false;
        } else {
            stopTagType = SUMO_TAG_STOP_CHARGINGSTATION;
        }
    } else if (stopParameters.parkingarea.size() > 0) {
        stoppingPlace = myNet->retrieveAdditional(SUMO_TAG_PARKING_AREA, stopParameters.parkingarea, false);
        // distinguish between stop for vehicles and stops for persons
        if (stopParent->getTagProperty().isPerson()) {
            WRITE_ERROR("Persons don't support stops at parking areas");
            validParentDemandElement = false;
        } else {
            stopTagType = SUMO_TAG_STOP_PARKINGAREA;
        }
    } else if (stopParameters.lane.size() > 0) {
        lane = myNet->retrieveLane(stopParameters.lane, false);
        stopTagType = SUMO_TAG_STOP_LANE;
    }
    // first check that parent is valid
    if (validParentDemandElement) {
        // check if values are correct
        if (stoppingPlace && lane) {
            WRITE_ERROR("A stop must be defined either over a stoppingPlace or over a lane");
        } else if (!stoppingPlace && !lane) {
            WRITE_ERROR("A stop requires a stoppingPlace or a lane");
        } else if (stoppingPlace) {
            // create stop using stopParameters and stoppingPlace
            GNEDemandElement* stop = new GNEStop(stopTagType, myNet, stopParameters, stoppingPlace, stopParent);
            // add it depending of undoDemandElements
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + stop->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(stop, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(stop);
                stoppingPlace->addChildElement(stop);
                stopParent->addChildElement(stop);
                stop->incRef("buildStoppingPlaceStop");
            }
        } else {
            // create stop using stopParameters and lane
            GNEDemandElement* stop = new GNEStop(myNet, stopParameters, lane, stopParent);
            // add it depending of undoDemandElements
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + stop->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(stop, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(stop);
                lane->addChildElement(stop);
                stopParent->addChildElement(stop);
                stop->incRef("buildLaneStop");
            }
        }
    }
}


void 
GNERouteHandler::buildPerson(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& personParameters,
    const std::map<std::string, std::string> &parameters) {
    // first check if ID is duplicated
    if (!isPersonIdDuplicated(myNet, personParameters.id)) {
        // obtain routes and vtypes
        GNEDemandElement* pType = myNet->retrieveDemandElement(SUMO_TAG_PTYPE, personParameters.vtypeid, false);
        if (pType == nullptr) {
            WRITE_ERROR("Invalid person type '" + personParameters.vtypeid + "' used in " + toString(personParameters.tag) + " '" + personParameters.id + "'.");
        } else {
            // create person using personParameters
            GNEDemandElement* person = new GNEPerson(SUMO_TAG_PERSON, myNet, pType, personParameters);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + person->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(person, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(person);
                // set person as child of pType and Route
                pType->addChildElement(person);
                person->incRef("buildPerson");
            }
        }
    }
}


void
GNERouteHandler::buildPersonFlow(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& personFlowParameters,
    const std::map<std::string, std::string> &parameters) {
    // first check if ID is duplicated
    if (!isPersonIdDuplicated(myNet, personFlowParameters.id)) {
        // obtain routes and vtypes
        GNEDemandElement* pType = myNet->retrieveDemandElement(SUMO_TAG_PTYPE, personFlowParameters.vtypeid, false);
        if (pType == nullptr) {
            WRITE_ERROR("Invalid personFlow type '" + personFlowParameters.vtypeid + "' used in " + toString(personFlowParameters.tag) + " '" + personFlowParameters.id + "'.");
        } else {
            // create personFlow using personFlowParameters
            GNEDemandElement* personFlow = new GNEPerson(SUMO_TAG_PERSONFLOW, myNet, pType, personFlowParameters);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + personFlow->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(personFlow, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(personFlow);
                // set personFlow as child of pType and Route
                pType->addChildElement(personFlow);
                personFlow->incRef("buildPersonFlow");
            }
        }
    }
}


void
GNERouteHandler::buildPersonTrip(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdgeID, const std::string &toEdgeID,
    const std::string &toBusStopID, double arrivalPos, const std::vector<std::string>& types, const std::vector<std::string>& modes) {
    // first parse parents
    GNEDemandElement* personParent = getPersonParent(sumoBaseObject);
    GNEEdge* fromEdge = myNet->retrieveEdge(fromEdgeID, false);
    GNEEdge* toEdge = myNet->retrieveEdge(toEdgeID, false);
    GNEAdditional* toBusStop = myNet->retrieveAdditional(SUMO_TAG_BUS_STOP, toBusStopID, false);
    // check conditions
    if (personParent && fromEdge) {
        if (toEdge) {
            // create personTrip from->to
            GNEDemandElement *personTrip = new GNEPersonTrip(myNet, personParent, fromEdge, toEdge, arrivalPos, types, modes);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + personTrip->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(personTrip, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(personTrip);
                // set child references
                personParent->addChildElement(personTrip);
                fromEdge->addChildElement(personTrip);
                toEdge->addChildElement(personTrip);
                personTrip->incRef("buildPersonTripFromTo");
            }
        } else if (toBusStop) {
            // create personTrip from->busStop
            GNEDemandElement *personTrip = new GNEPersonTrip(myNet, personParent, fromEdge, toBusStop, arrivalPos, types, modes);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + personTrip->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(personTrip, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(personTrip);
                // set child references
                personParent->addChildElement(personTrip);
                fromEdge->addChildElement(personTrip);
                toBusStop->addChildElement(personTrip);
                personTrip->incRef("buildPersonTripFromBusStop");
            }
        }
    }
}


void
GNERouteHandler::buildWalk(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdgeID, const std::string &toEdgeID,
    const std::string &toBusStopID, const std::vector<std::string>& edgeIDs, const std::string &routeID, double arrivalPos) {
    // first parse parents
    GNEDemandElement* personParent = getPersonParent(sumoBaseObject);
    GNEEdge* fromEdge = myNet->retrieveEdge(fromEdgeID, false);
    GNEEdge* toEdge = myNet->retrieveEdge(toEdgeID, false);
    GNEAdditional* toBusStop = myNet->retrieveAdditional(SUMO_TAG_BUS_STOP, toBusStopID, false);
    GNEDemandElement* route = myNet->retrieveDemandElement(SUMO_TAG_ROUTE, routeID, false);
    std::vector<GNEEdge*> edges = parseEdges(SUMO_TAG_WALK, edgeIDs);
    // check conditions
    if (personParent && fromEdge) {
        if (edges.size() > 0) {
            // create walk edges
            GNEDemandElement *walk = new GNEWalk(myNet, personParent, edges, arrivalPos);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + walk->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(walk, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(walk);
                // set child references
                personParent->addChildElement(walk);
                for (const auto &edge : edges) {
                    edge->addChildElement(walk);
                }
                walk->incRef("buildWalkEdges");
            }
        } else if (route) {
            // create walk over route
            GNEDemandElement *walk = new GNEWalk(myNet, personParent, route, arrivalPos);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + walk->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(walk, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(walk);
                // set child references
                personParent->addChildElement(walk);
                route->addChildElement(walk);
                walk->incRef("buildWalkRoute");
            }
        } else if (toEdge) {
            // create walk from->to
            GNEDemandElement *walk = new GNEWalk(myNet, personParent, fromEdge, toEdge, arrivalPos);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + walk->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(walk, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(walk);
                // set child references
                personParent->addChildElement(walk);
                fromEdge->addChildElement(walk);
                toEdge->addChildElement(walk);
                walk->incRef("buildWalkFromTo");
            }
        } else if (toBusStop) {
            // create walk from->busStop
            GNEDemandElement *walk = new GNEWalk(myNet, personParent, fromEdge, toBusStop, arrivalPos);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + walk->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(walk, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(walk);
                // set child references
                personParent->addChildElement(walk);
                fromEdge->addChildElement(walk);
                toBusStop->addChildElement(walk);
                walk->incRef("buildWalkFromBusStop");
            }
        }
    }
}


void
GNERouteHandler::buildRide(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdgeID, const std::string &toEdgeID, 
    const std::string &toBusStopID, double arrivalPos, const std::vector<std::string>& lines) {
    // first parse parents
    GNEDemandElement* personParent = getPersonParent(sumoBaseObject);
    GNEEdge* fromEdge = myNet->retrieveEdge(fromEdgeID, false);
    GNEEdge* toEdge = myNet->retrieveEdge(toEdgeID, false);
    GNEAdditional* toBusStop = myNet->retrieveAdditional(SUMO_TAG_BUS_STOP, toBusStopID, false);
    // check conditions
    if (personParent && fromEdge) {
        if (toEdge) {
            // create ride from->to
            GNEDemandElement *ride = new GNERide(myNet, personParent, fromEdge, toEdge, arrivalPos, lines);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + ride->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(ride, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(ride);
                // set child references
                personParent->addChildElement(ride);
                fromEdge->addChildElement(ride);
                toEdge->addChildElement(ride);
                ride->incRef("buildRideFromTo");
            }
        } else if (toBusStop) {
            // create ride from->busStop
            GNEDemandElement *ride = new GNERide(myNet, personParent, fromEdge, toBusStop, arrivalPos, lines);
            if (myUndoDemandElements) {
                myNet->getViewNet()->getUndoList()->p_begin("add " + ride->getTagStr());
                myNet->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(ride, true), true);
                myNet->getViewNet()->getUndoList()->p_end();
            } else {
                myNet->getAttributeCarriers()->insertDemandElement(ride);
                // set child references
                personParent->addChildElement(ride);
                fromEdge->addChildElement(ride);
                toBusStop->addChildElement(ride);
                ride->incRef("buildRideFromBusStop");
            }
        }
    }
}


void
GNERouteHandler::buildContainer(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& containerParameters,
    const std::map<std::string, std::string> &parameters) {
    //
}


void
GNERouteHandler::buildContainerFlow(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& containerFlowParameters,
    const std::map<std::string, std::string> &parameters) {
    //
}


void 
GNERouteHandler::buildTransport(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge,
    const std::string &toBusStop, const std::vector<std::string>& lines, const double arrivalPos) {
    //
}


void
GNERouteHandler::buildTranship(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge,
    const std::string &toBusStop, const std::vector<std::string>& edges, const double speed, const double departPosition, const double arrivalPosition) {
    //
}


bool
GNERouteHandler::isVehicleIdDuplicated(GNENet* net, const std::string& id) {
    // declare vehicle tags vector
    std::vector<SumoXMLTag> vehicleTags = {SUMO_TAG_VEHICLE, GNE_TAG_VEHICLE_WITHROUTE, SUMO_TAG_TRIP, GNE_TAG_FLOW_ROUTE, GNE_TAG_FLOW_WITHROUTE, SUMO_TAG_FLOW};
    for (const auto& vehicleTag : vehicleTags) {
        if (net->retrieveDemandElement(vehicleTag, id, false) != nullptr) {
            WRITE_ERROR("There is another " + toString(vehicleTag) + " with the same ID='" + id + "'.");
            return true;
        }
    }
    return false;
}


bool
GNERouteHandler::isPersonIdDuplicated(GNENet* net, const std::string& id) {
    for (SumoXMLTag personTag : std::vector<SumoXMLTag>({SUMO_TAG_PERSON, SUMO_TAG_PERSONFLOW})) {
        if (net->retrieveDemandElement(personTag, id, false) != nullptr) {
            WRITE_ERROR("There is another " + toString(personTag) + " with the same ID='" + id + "'.");
            return true;
        }
    }
    return false;
}


bool
GNERouteHandler::isContainerIdDuplicated(GNENet* net, const std::string& id) {
    for (SumoXMLTag containerTag : std::vector<SumoXMLTag>({SUMO_TAG_CONTAINER, SUMO_TAG_CONTAINERFLOW})) {
        if (net->retrieveDemandElement(containerTag, id, false) != nullptr) {
            WRITE_ERROR("There is another " + toString(containerTag) + " with the same ID='" + id + "'.");
            return true;
        }
    }
    return false;
}


bool
GNERouteHandler::buildPersonPlan(SumoXMLTag tag, GNEDemandElement* personParent, GNEFrameAttributesModuls::AttributesCreator* personPlanAttributes, GNEFrameModuls::PathCreator* pathCreator) {
    // get view net
    GNEViewNet* viewNet = personParent->getNet()->getViewNet();
    // Declare map to keep attributes from myPersonPlanAttributes
    std::map<SumoXMLAttr, std::string> valuesMap = personPlanAttributes->getAttributesAndValuesTemporal(true);
    // get attributes
    const std::vector<std::string> types = GNEAttributeCarrier::parse<std::vector<std::string> >(valuesMap[SUMO_ATTR_VTYPES]);
    const std::vector<std::string> modes = GNEAttributeCarrier::parse<std::vector<std::string> >(valuesMap[SUMO_ATTR_MODES]);
    const std::vector<std::string> lines = GNEAttributeCarrier::parse<std::vector<std::string> >(valuesMap[SUMO_ATTR_LINES]);
    const double arrivalPos = (valuesMap.count(SUMO_ATTR_ARRIVALPOS) > 0) ? GNEAttributeCarrier::parse<double>(valuesMap[SUMO_ATTR_ARRIVALPOS]) : 0;
    // get stop parameters
    SUMOVehicleParameter::Stop stopParameters;
    // get edges
    GNEEdge* fromEdge = (pathCreator->getSelectedEdges().size() > 0) ? pathCreator->getSelectedEdges().front() : nullptr;
    GNEEdge* toEdge = (pathCreator->getSelectedEdges().size() > 0) ? pathCreator->getSelectedEdges().back() : nullptr;
    // get busStop
    GNEAdditional* toBusStop = pathCreator->getToStoppingPlace(SUMO_TAG_BUS_STOP);
    // get path edges
    std::vector<GNEEdge*> edges;
    for (const auto& path : pathCreator->getPath()) {
        for (const auto& edge : path.getSubPath()) {
            edges.push_back(edge);
        }
    }
    // get route
    GNEDemandElement* route = pathCreator->getRoute();
    // check what PersonPlan we're creating
    switch (tag) {
        // Person Trips
        case GNE_TAG_PERSONTRIP_EDGE: {
            // check if person trip busStop->edge can be created
            if (fromEdge && toEdge) {
                buildPersonTrip(viewNet->getNet(), true, personParent, fromEdge, toEdge, nullptr, arrivalPos, types, modes);
                return true;
            } else {
                viewNet->setStatusBarText("A person trip from edge to edge needs two edges edge");
            }
            break;
        }
        case GNE_TAG_PERSONTRIP_BUSSTOP: {
            // check if person trip busStop->busStop can be created
            if (fromEdge && toBusStop) {
                buildPersonTrip(viewNet->getNet(), true, personParent, fromEdge, nullptr, toBusStop, arrivalPos, types, modes);
                return true;
            } else {
                viewNet->setStatusBarText("A person trip from edge to busStop needs one edge and one busStop");
            }
            break;
        }
        // Walks
        case GNE_TAG_WALK_EDGE: {
            // check if transport busStop->edge can be created
            if (fromEdge && toEdge) {
                buildWalk(viewNet->getNet(), true, personParent, fromEdge, toEdge, nullptr, {}, nullptr, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A ride from busStop to edge needs a busStop and an edge");
            }
            break;
        }
        case GNE_TAG_WALK_BUSSTOP: {
            // check if transport busStop->busStop can be created
            if (fromEdge && toBusStop) {
                buildWalk(viewNet->getNet(), true, personParent, fromEdge, nullptr, toBusStop, {}, nullptr, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A transport from busStop to busStop needs two busStops");
            }
            break;
        }
        case GNE_TAG_WALK_EDGES: {
            // check if transport edges can be created
            if (edges.size() > 0) {
                buildWalk(viewNet->getNet(), true, personParent, nullptr, nullptr, nullptr, edges, nullptr, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A transport with edges attribute needs a list of edges");
            }
            break;
        }
        case GNE_TAG_WALK_ROUTE: {
            // check if transport edges can be created
            if (route) {
                buildWalk(viewNet->getNet(), true, personParent, nullptr, nullptr, nullptr, {}, route, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A route transport needs a route");
            }
            break;
        }
        // Rides
        case GNE_TAG_RIDE_EDGE: {
            // check if ride busStop->edge can be created
            if (fromEdge && toEdge) {
                buildRide(viewNet->getNet(), true, personParent, fromEdge, toEdge, nullptr, arrivalPos, lines);
                return true;
            } else {
                viewNet->setStatusBarText("A ride from edge to edge needs two edges edge");
            }
            break;
        }
        case GNE_TAG_RIDE_BUSSTOP: {
            // check if ride busStop->busStop can be created
            if (fromEdge && toBusStop) {
                buildRide(viewNet->getNet(), true, personParent, fromEdge, nullptr, toBusStop, arrivalPos, lines);
                return true;
            } else {
                viewNet->setStatusBarText("A ride from edge to busStop needs one edge and one busStop");
            }
            break;
        }
        // stops
        case GNE_TAG_STOPPERSON_EDGE: {
            // check if ride busStop->busStop can be created
            if (fromEdge) {
                stopParameters.edge = fromEdge->getID();
                buildStopPerson(viewNet->getNet(), true, personParent, fromEdge, nullptr, stopParameters);
                return true;
            } else {
                viewNet->setStatusBarText("A stop has to be placed over an edge");
            }
            break;
        }
        case GNE_TAG_STOPPERSON_BUSSTOP: {
            // check if ride busStop->busStop can be created
            if (toBusStop) {
                stopParameters.busstop = toBusStop->getID();
                buildStopPerson(viewNet->getNet(), true, personParent, nullptr, toBusStop, stopParameters);
                return true;
            } else {
                viewNet->setStatusBarText("A stop has to be placed over a busStop");
            }
            break;
        }
        default:
            throw InvalidArgument("Invalid person plan tag");
    }
    // person plan wasn't created, then return false
    return false;
}


void
GNERouteHandler::buildPersonTrip(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge, GNEAdditional* toBusStop,
                                 double arrivalPos, const std::vector<std::string>& types, const std::vector<std::string>& modes) {
    // declare person trip
    GNEDemandElement* personTrip = nullptr;
    // create person trip depending of parameters
    if (fromEdge && toEdge) {
        // create person trip edge->edge
        personTrip = new GNEPersonTrip(net, personParent, fromEdge, toEdge, arrivalPos, types, modes);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_PERSONTRIP_EDGE) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(personTrip, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert person trip
            net->getAttributeCarriers()->insertDemandElement(personTrip);
            // set references in children
            personParent->addChildElement(personTrip);
            fromEdge->addChildElement(personTrip);
            toEdge->addChildElement(personTrip);
            // include reference
            personTrip->incRef("buildPersonTrip");
        }
    } else if (fromEdge && toBusStop) {
        // create person trip edge->busStop
        personTrip = new GNEPersonTrip(net, personParent, fromEdge, toBusStop, arrivalPos, types, modes);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_PERSONTRIP_BUSSTOP) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(personTrip, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert person trip
            net->getAttributeCarriers()->insertDemandElement(personTrip);
            // set references in children
            personParent->addChildElement(personTrip);
            fromEdge->addChildElement(personTrip);
            toBusStop->addChildElement(personTrip);
            // include reference
            personTrip->incRef("buildPersonTrip");
        }
    }
    // update geometry
    personParent->updateGeometry();
}


void
GNERouteHandler::buildWalk(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge,
                           GNEAdditional* toBusStop, const std::vector<GNEEdge*>& edges, GNEDemandElement* route, double arrivalPos) {
    // declare transport
    GNEDemandElement* transport = nullptr;
    // create transport depending of parameters
    if (fromEdge && toEdge) {
        // create transport edge->edge
        transport = new GNEWalk(net, personParent, fromEdge, toEdge, arrivalPos);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_WALK_EDGE) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(transport, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert transport
            net->getAttributeCarriers()->insertDemandElement(transport);
            // set references in children
            personParent->addChildElement(transport);
            fromEdge->addChildElement(transport);
            toEdge->addChildElement(transport);
            // include reference
            transport->incRef("buildWalk");
        }
    } else if (fromEdge && toBusStop) {
        // create transport edge->busStop
        transport = new GNEWalk(net, personParent, fromEdge, toBusStop, arrivalPos);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_WALK_BUSSTOP) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(transport, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert transport
            net->getAttributeCarriers()->insertDemandElement(transport);
            // set references in children
            personParent->addChildElement(transport);
            fromEdge->addChildElement(transport);
            toBusStop->addChildElement(transport);
            // include reference
            transport->incRef("buildWalk");
        }
    } else if (edges.size() > 0) {
        // create transport edges
        transport = new GNEWalk(net, personParent, edges, arrivalPos);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_WALK_EDGES) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(transport, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert transport
            net->getAttributeCarriers()->insertDemandElement(transport);
            // set references in children
            personParent->addChildElement(transport);
            for (const auto& edge : edges) {
                edge->addChildElement(transport);
            }
            // include reference
            transport->incRef("buildWalk");
        }
    } else if (route) {
        // create transport edges
        transport = new GNEWalk(net, personParent, route, arrivalPos);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_WALK_ROUTE) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(transport, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert transport
            net->getAttributeCarriers()->insertDemandElement(transport);
            // set references in children
            personParent->addChildElement(transport);
            route->addChildElement(transport);
            // include reference
            transport->incRef("buildWalk");
        }
    }
    // update geometry
    personParent->updateGeometry();
}


void
GNERouteHandler::buildRide(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge, GNEAdditional* toBusStop,
                           double arrivalPos, const std::vector<std::string>& lines) {
    // declare ride
    GNEDemandElement* ride = nullptr;
    // create ride depending of parameters
    if (fromEdge && toEdge) {
        // create ride edge->edge
        ride = new GNERide(net, personParent, fromEdge, toEdge, arrivalPos, lines);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_RIDE_EDGE) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(ride, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert ride
            net->getAttributeCarriers()->insertDemandElement(ride);
            // set references in children
            personParent->addChildElement(ride);
            fromEdge->addChildElement(ride);
            toEdge->addChildElement(ride);
            // include reference
            ride->incRef("buildRide");
        }
    } else if (fromEdge && toBusStop) {
        // create ride edge->busStop
        ride = new GNERide(net, personParent, fromEdge, toBusStop, arrivalPos, lines);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_RIDE_BUSSTOP) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(ride, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert ride
            net->getAttributeCarriers()->insertDemandElement(ride);
            // set references in children
            personParent->addChildElement(ride);
            fromEdge->addChildElement(ride);
            toBusStop->addChildElement(ride);
            // include reference
            ride->incRef("buildRide");
        }
    }
    // update geometry
    personParent->updateGeometry();
}


void
GNERouteHandler::buildStopPerson(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* edge, GNEAdditional* busStop, const SUMOVehicleParameter::Stop& stopParameters) {
    // declare stopPerson
    GNEDemandElement* stopPerson = nullptr;
    // create stopPerson depending of parameters
    if (edge) {
        // create stopPerson over edge
        stopPerson = new GNEStopPerson(net, personParent, edge, stopParameters);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_STOPPERSON_EDGE) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(stopPerson, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert stopPerson
            net->getAttributeCarriers()->insertDemandElement(stopPerson);
            // set references in children
            personParent->addChildElement(stopPerson);
            edge->addChildElement(stopPerson);
            // include reference
            stopPerson->incRef("buildStopPerson");
        }
    } else if (busStop) {
        // create stopPerson over busStop
        stopPerson = new GNEStopPerson(net, personParent, busStop, stopParameters);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_STOPPERSON_BUSSTOP) + " within person '" + personParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(stopPerson, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert stopPerson
            net->getAttributeCarriers()->insertDemandElement(stopPerson);
            // set references in children
            personParent->addChildElement(stopPerson);
            busStop->addChildElement(stopPerson);
            // include reference
            stopPerson->incRef("buildStopPerson");
        }
    }
    // update geometry
    personParent->updateGeometry();
}


void
GNERouteHandler::buildContainer(GNENet* net, bool undoDemandElements, const SUMOVehicleParameter& containerParameters) {
    // first check if ID is duplicated
    if (!isContainerIdDuplicated(net, containerParameters.id)) {
        // obtain routes and vtypes
        GNEDemandElement* pType = net->retrieveDemandElement(SUMO_TAG_PTYPE, containerParameters.vtypeid, false);
        if (pType == nullptr) {
            WRITE_ERROR("Invalid container type '" + containerParameters.vtypeid + "' used in " + toString(containerParameters.tag) + " '" + containerParameters.id + "'.");
        } else {
            // create container using containerParameters
            GNEDemandElement* container = new GNEContainer(SUMO_TAG_CONTAINER, net, pType, containerParameters);
            if (undoDemandElements) {
                net->getViewNet()->getUndoList()->p_begin("add " + container->getTagStr());
                net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(container, true), true);
                net->getViewNet()->getUndoList()->p_end();
            } else {
                net->getAttributeCarriers()->insertDemandElement(container);
                // set container as child of pType and Route
                pType->addChildElement(container);
                container->incRef("buildContainer");
            }
        }
    }
}


void
GNERouteHandler::buildContainerFlow(GNENet* net, bool undoDemandElements, const SUMOVehicleParameter& containerFlowParameters) {
    // first check if ID is duplicated
    if (!isContainerIdDuplicated(net, containerFlowParameters.id)) {
        // obtain routes and vtypes
        GNEDemandElement* pType = net->retrieveDemandElement(SUMO_TAG_PTYPE, containerFlowParameters.vtypeid, false);
        if (pType == nullptr) {
            WRITE_ERROR("Invalid containerFlow type '" + containerFlowParameters.vtypeid + "' used in " + toString(containerFlowParameters.tag) + " '" + containerFlowParameters.id + "'.");
        } else {
            // create containerFlow using containerFlowParameters
            GNEDemandElement* containerFlow = new GNEContainer(SUMO_TAG_CONTAINERFLOW, net, pType, containerFlowParameters);
            if (undoDemandElements) {
                net->getViewNet()->getUndoList()->p_begin("add " + containerFlow->getTagStr());
                net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(containerFlow, true), true);
                net->getViewNet()->getUndoList()->p_end();
            } else {
                net->getAttributeCarriers()->insertDemandElement(containerFlow);
                // set containerFlow as child of pType and Route
                pType->addChildElement(containerFlow);
                containerFlow->incRef("buildContainerFlow");
            }
        }
    }
}


bool
GNERouteHandler::buildContainerPlan(SumoXMLTag tag, GNEDemandElement* containerParent, GNEFrameAttributesModuls::AttributesCreator* containerPlanAttributes, GNEFrameModuls::PathCreator* pathCreator) {
    // get view net
    GNEViewNet* viewNet = containerParent->getNet()->getViewNet();
    // Declare map to keep attributes from myContainerPlanAttributes
    std::map<SumoXMLAttr, std::string> valuesMap = containerPlanAttributes->getAttributesAndValuesTemporal(true);
    // get attributes
    const std::vector<std::string> lines = GNEAttributeCarrier::parse<std::vector<std::string> >(valuesMap[SUMO_ATTR_LINES]);
    const double speed = (valuesMap.count(SUMO_ATTR_SPEED) > 0) ? GNEAttributeCarrier::parse<double>(valuesMap[SUMO_ATTR_SPEED]) : 1.39;
    const double departPos = (valuesMap.count(SUMO_ATTR_DEPARTPOS) > 0) ? GNEAttributeCarrier::parse<double>(valuesMap[SUMO_ATTR_DEPARTPOS]) : 0;
    const double arrivalPos = (valuesMap.count(SUMO_ATTR_ARRIVALPOS) > 0) ? GNEAttributeCarrier::parse<double>(valuesMap[SUMO_ATTR_ARRIVALPOS]) : 0;
    // get stop parameters
    SUMOVehicleParameter::Stop stopParameters;
    // get edges
    GNEEdge* fromEdge = (pathCreator->getSelectedEdges().size() > 0) ? pathCreator->getSelectedEdges().front() : nullptr;
    GNEEdge* toEdge = (pathCreator->getSelectedEdges().size() > 0) ? pathCreator->getSelectedEdges().back() : nullptr;
    // get busStop
    GNEAdditional* toBusStop = pathCreator->getToStoppingPlace(SUMO_TAG_CONTAINER_STOP);
    // get path edges
    std::vector<GNEEdge*> edges;
    for (const auto& path : pathCreator->getPath()) {
        for (const auto& edge : path.getSubPath()) {
            edges.push_back(edge);
        }
    }
    // check what ContainerPlan we're creating
    switch (tag) {
        // Transports
        case GNE_TAG_TRANSPORT_EDGE: {
            // check if transport busStop->edge can be created
            if (fromEdge && toEdge) {
                buildTransport(viewNet->getNet(), true, containerParent, fromEdge, toEdge, nullptr, lines, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A ride from busStop to edge needs a busStop and an edge");
            }
            break;
        }
        case GNE_TAG_TRANSPORT_CONTAINERSTOP: {
            // check if transport busStop->busStop can be created
            if (fromEdge && toBusStop) {
                buildTransport(viewNet->getNet(), true, containerParent, fromEdge, nullptr, toBusStop, lines, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A transport from busStop to busStop needs two busStops");
            }
            break;
        }
        // Tranships
        case GNE_TAG_TRANSHIP_EDGE: {
            // check if tranship busStop->edge can be created
            if (fromEdge && toEdge) {
                buildTranship(viewNet->getNet(), true, containerParent, fromEdge, toEdge, nullptr, {}, speed, departPos, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A ride from busStop to edge needs a busStop and an edge");
            }
            break;
        }
        case GNE_TAG_TRANSHIP_CONTAINERSTOP: {
            // check if tranship busStop->busStop can be created
            if (fromEdge && toBusStop) {
                buildTranship(viewNet->getNet(), true, containerParent, fromEdge, nullptr, toBusStop, {}, speed, departPos, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A tranship from busStop to busStop needs two busStops");
            }
            break;
        }
        case GNE_TAG_TRANSHIP_EDGES: {
            // check if tranship edges can be created
            if (edges.size() > 0) {
                buildTranship(viewNet->getNet(), true, containerParent, nullptr, nullptr, nullptr, edges, speed, departPos, arrivalPos);
                return true;
            } else {
                viewNet->setStatusBarText("A tranship with edges attribute needs a list of edges");
            }
            break;
        }
        // stops
        case GNE_TAG_STOPCONTAINER_EDGE: {
            // check if ride busStop->busStop can be created
            if (fromEdge) {
                stopParameters.edge = fromEdge->getID();
                buildStopContainer(viewNet->getNet(), true, containerParent, fromEdge, nullptr, stopParameters);
                return true;
            } else {
                viewNet->setStatusBarText("A stop has to be placed over an edge");
            }
            break;
        }
        case GNE_TAG_STOPCONTAINER_CONTAINERSTOP: {
            // check if ride busStop->busStop can be created
            if (toBusStop) {
                stopParameters.busstop = toBusStop->getID();
                buildStopContainer(viewNet->getNet(), true, containerParent, nullptr, toBusStop, stopParameters);
                return true;
            } else {
                viewNet->setStatusBarText("A stop has to be placed over a busStop");
            }
            break;
        }
        default:
            throw InvalidArgument("Invalid container plan tag");
    }
    // container plan wasn't created, then return false
    return false;
}


void
GNERouteHandler::buildTransport(GNENet* net, bool undoDemandElements, GNEDemandElement* containerParent, GNEEdge* fromEdge, GNEEdge* toEdge,
                           GNEAdditional* toBusStop, const std::vector<std::string>& lines, const double arrivalPos) {
    // declare transport
    GNEDemandElement* transport = nullptr;
    // create transport depending of parameters
    if (fromEdge && toEdge) {
        // create transport edge->edge
        transport = new GNETransport(net, containerParent, fromEdge, toEdge, lines, arrivalPos);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_TRANSPORT_EDGE) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(transport, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert transport
            net->getAttributeCarriers()->insertDemandElement(transport);
            // set references in children
            containerParent->addChildElement(transport);
            fromEdge->addChildElement(transport);
            toEdge->addChildElement(transport);
            // include reference
            transport->incRef("buildTransport");
        }
    } else if (fromEdge && toBusStop) {
        // create transport edge->busStop
        transport = new GNETransport(net, containerParent, fromEdge, toBusStop, lines, arrivalPos);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_TRANSPORT_CONTAINERSTOP) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(transport, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert transport
            net->getAttributeCarriers()->insertDemandElement(transport);
            // set references in children
            containerParent->addChildElement(transport);
            fromEdge->addChildElement(transport);
            toBusStop->addChildElement(transport);
            // include reference
            transport->incRef("buildTransport");
        }
    }
    // update geometry
    containerParent->updateGeometry();
}


void
GNERouteHandler::buildTranship(GNENet* net, bool undoDemandElements, GNEDemandElement* containerParent, GNEEdge* fromEdge, GNEEdge* toEdge,
                           GNEAdditional* toBusStop, const std::vector<GNEEdge*>& edges, const double speed, double departPosition, double arrivalPosition) {
    // declare tranship
    GNEDemandElement* tranship = nullptr;
    // create tranship depending of parameters
    if (fromEdge && toEdge) {
        // create tranship edge->edge
        tranship = new GNETranship(net, containerParent, fromEdge, toEdge, speed, departPosition, arrivalPosition);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_TRANSHIP_EDGE) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(tranship, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert tranship
            net->getAttributeCarriers()->insertDemandElement(tranship);
            // set references in children
            containerParent->addChildElement(tranship);
            fromEdge->addChildElement(tranship);
            toEdge->addChildElement(tranship);
            // include reference
            tranship->incRef("buildTranship");
        }
    } else if (fromEdge && toBusStop) {
        // create tranship edge->busStop
        tranship = new GNETranship(net, containerParent, fromEdge, toBusStop, speed, departPosition, arrivalPosition);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_TRANSHIP_CONTAINERSTOP) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(tranship, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert tranship
            net->getAttributeCarriers()->insertDemandElement(tranship);
            // set references in children
            containerParent->addChildElement(tranship);
            fromEdge->addChildElement(tranship);
            toBusStop->addChildElement(tranship);
            // include reference
            tranship->incRef("buildTranship");
        }
    } else if (edges.size() > 0) {
        // create tranship edges
        tranship = new GNETranship(net, containerParent, edges, speed, departPosition, arrivalPosition);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_TRANSHIP_EDGES) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(tranship, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert tranship
            net->getAttributeCarriers()->insertDemandElement(tranship);
            // set references in children
            containerParent->addChildElement(tranship);
            for (const auto& edge : edges) {
                edge->addChildElement(tranship);
            }
            // include reference
            tranship->incRef("buildTranship");
        }
    }
    // update geometry
    containerParent->updateGeometry();
}




void
GNERouteHandler::buildStopContainer(GNENet* net, bool undoDemandElements, GNEDemandElement* containerParent, GNEEdge* edge, GNEAdditional* containerStop, const SUMOVehicleParameter::Stop& stopParameters) {
    // declare stopContainer
    GNEDemandElement* stopContainer = nullptr;
    // create stopContainer depending of parameters
    if (edge) {
        // create stopContainer over edge
        stopContainer = new GNEStopContainer(net, containerParent, edge, stopParameters);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(GNE_TAG_STOPCONTAINER_EDGE) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(stopContainer, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert stopContainer
            net->getAttributeCarriers()->insertDemandElement(stopContainer);
            // set references in children
            containerParent->addChildElement(stopContainer);
            edge->addChildElement(stopContainer);
            // include reference
            stopContainer->incRef("buildStopContainer");
        }
    } else if (containerStop) {
        // create stopContainer over containerStop
        stopContainer = new GNEStopContainer(net, containerParent, containerStop, stopParameters);
        // add element using undo list or directly, depending of undoDemandElements flag
        if (undoDemandElements) {
            net->getViewNet()->getUndoList()->p_begin("add " + toString(SUMO_TAG_STOP_CONTAINERSTOP) + " within container '" + containerParent->getID() + "'");
            net->getViewNet()->getUndoList()->add(new GNEChange_DemandElement(stopContainer, true), true);
            net->getViewNet()->getUndoList()->p_end();
        } else {
            // insert stopContainer
            net->getAttributeCarriers()->insertDemandElement(stopContainer);
            // set references in children
            containerParent->addChildElement(stopContainer);
            containerStop->addChildElement(stopContainer);
            // include reference
            stopContainer->incRef("buildStopContainer");
        }
    }
    // update geometry
    containerParent->updateGeometry();
}


void
GNERouteHandler::transformToVehicle(GNEVehicle* originalVehicle, bool createEmbeddedRoute) {
    // get original vehicle tag
    SumoXMLTag tag = originalVehicle->getTagProperty().getTag();
    // get pointer to net
    GNENet* net = originalVehicle->getNet();
    // obtain vehicle parameters
    SUMOVehicleParameter vehicleParameters = *originalVehicle;
    // set "yellow" as original route color
    RGBColor routeColor = RGBColor::YELLOW;
    // declare edges
    std::vector<GNEEdge*> routeEdges;
    // obtain edges depending of tag
    if ((tag == GNE_TAG_FLOW_ROUTE) || (tag == SUMO_TAG_VEHICLE)) {
        // get route edges
        routeEdges = originalVehicle->getParentDemandElements().back()->getParentEdges();
        // get original route color
        routeColor = originalVehicle->getParentDemandElements().back()->getColor();
    } else if ((tag == GNE_TAG_VEHICLE_WITHROUTE) || (tag == GNE_TAG_FLOW_WITHROUTE)) {
        // get embedded route edges
        routeEdges = originalVehicle->getChildDemandElements().front()->getParentEdges();
    } else if ((tag == SUMO_TAG_TRIP) || (tag == SUMO_TAG_FLOW)) {
        // calculate path using from-via-to edges
        routeEdges = originalVehicle->getNet()->getPathManager()->getPathCalculator()->calculateDijkstraPath(originalVehicle->getVClass(), originalVehicle->getParentEdges());
    }
    // only continue if edges are valid
    if (routeEdges.empty()) {
        // declare header
        const std::string header = "Problem transforming to vehicle";
        // declare message
        const std::string message = "Vehicle cannot be transformed. Invalid number of edges";
        // write warning
        WRITE_DEBUG("Opened FXMessageBox " + header);
        // open message box
        FXMessageBox::warning(originalVehicle->getNet()->getViewNet()->getApp(), MBOX_OK, header.c_str(), "%s", message.c_str());
        // write warning if netedit is running in testing mode
        WRITE_DEBUG("Closed FXMessageBox " + header);
    } else {
        // begin undo-redo operation
        net->getViewNet()->getUndoList()->p_begin("transform " + originalVehicle->getTagStr() + " to " + toString(SUMO_TAG_VEHICLE));
        // first delete vehicle
        net->deleteDemandElement(originalVehicle, net->getViewNet()->getUndoList());
        // check if new vehicle must have an embedded route
        if (createEmbeddedRoute) {
            // change tag in vehicle parameters
            vehicleParameters.tag = GNE_TAG_VEHICLE_WITHROUTE;
            // create a vehicle with embebbed routes
/*
            buildVehicleEmbeddedRoute(net, true, vehicleParameters, routeParameters.edges);
*/
        } else {
            // change tag in vehicle parameters
            vehicleParameters.tag = SUMO_TAG_VEHICLE;
            // generate a new route id
            const std::string routeID = net->generateDemandElementID(SUMO_TAG_ROUTE);
            // build route
/*
            buildRoute(net, true, routeParameters, {});
*/
            // set route ID in vehicle parameters
            vehicleParameters.routeid = routeID;
            // create vehicle
/*
            buildVehicleOverRoute(net, true, vehicleParameters);
*/
        }
        // end undo-redo operation
        net->getViewNet()->getUndoList()->p_end();
    }
}


void
GNERouteHandler::transformToRouteFlow(GNEVehicle* originalVehicle, bool createEmbeddedRoute) {
    // get original vehicle tag
    SumoXMLTag tag = originalVehicle->getTagProperty().getTag();
    // get pointer to net
    GNENet* net = originalVehicle->getNet();
    // obtain vehicle parameters
    SUMOVehicleParameter vehicleParameters = *originalVehicle;
    // set "yellow" as original route color
    RGBColor routeColor = RGBColor::YELLOW;
    // declare edges
    std::vector<GNEEdge*> routeEdges;
    // obtain edges depending of tag
    if ((tag == GNE_TAG_FLOW_ROUTE) || (tag == SUMO_TAG_VEHICLE)) {
        // get route edges
        routeEdges = originalVehicle->getParentDemandElements().back()->getParentEdges();
        // get original route color
        routeColor = originalVehicle->getParentDemandElements().back()->getColor();
    } else if ((tag == GNE_TAG_VEHICLE_WITHROUTE) || (tag == GNE_TAG_FLOW_WITHROUTE)) {
        // get embedded route edges
        routeEdges = originalVehicle->getChildDemandElements().front()->getParentEdges();
    } else if ((tag == SUMO_TAG_TRIP) || (tag == SUMO_TAG_FLOW)) {
        // calculate path using from-via-to edges
        routeEdges = originalVehicle->getNet()->getPathManager()->getPathCalculator()->calculateDijkstraPath(originalVehicle->getVClass(), originalVehicle->getParentEdges());
    }
    // only continue if edges are valid
    if (routeEdges.empty()) {
        // declare header
        const std::string header = "Problem transforming to vehicle";
        // declare message
        const std::string message = "Vehicle cannot be transformed. Invalid number of edges";
        // write warning
        WRITE_DEBUG("Opened FXMessageBox " + header);
        // open message box
        FXMessageBox::warning(originalVehicle->getNet()->getViewNet()->getApp(), MBOX_OK, header.c_str(), "%s", message.c_str());
        // write warning if netedit is running in testing mode
        WRITE_DEBUG("Closed FXMessageBox " + header);
    } else {
        // begin undo-redo operation
        net->getViewNet()->getUndoList()->p_begin("transform " + originalVehicle->getTagStr() + " to " + toString(SUMO_TAG_VEHICLE));
        // first delete vehicle
        net->deleteDemandElement(originalVehicle, net->getViewNet()->getUndoList());
        // change depart
        if ((vehicleParameters.tag == SUMO_TAG_TRIP) || (vehicleParameters.tag == SUMO_TAG_VEHICLE) || (vehicleParameters.tag == GNE_TAG_VEHICLE_WITHROUTE)) {
            // set end
            vehicleParameters.repetitionEnd = vehicleParameters.depart + 3600;
            // set number
            vehicleParameters.repetitionNumber = 1800;
            vehicleParameters.parametersSet |= VEHPARS_NUMBER_SET;
            // unset parameters
            vehicleParameters.parametersSet &= ~VEHPARS_END_SET;
            vehicleParameters.parametersSet &= ~VEHPARS_VPH_SET;
            vehicleParameters.parametersSet &= ~VEHPARS_PERIOD_SET;
            vehicleParameters.parametersSet &= ~VEHPARS_PROB_SET;
        }
        // check if new vehicle must have an embedded route
        if (createEmbeddedRoute) {
            // change tag in vehicle parameters
            vehicleParameters.tag = GNE_TAG_FLOW_WITHROUTE;
            // create a flow with embebbed routes
/*
            buildFlowEmbeddedRoute(net, true, vehicleParameters, routeParameters.edges);
*/
        } else {
            // change tag in vehicle parameters
            vehicleParameters.tag = GNE_TAG_FLOW_ROUTE;
            // generate a new route id
            const std::string routeID = net->generateDemandElementID(SUMO_TAG_ROUTE);
            // build route
/*
            buildRoute(net, true, routeParameters, {});
*/
            // set route ID in vehicle parameters
            vehicleParameters.routeid = routeID;
            // create flow
/*
            buildFlowOverRoute(net, true, vehicleParameters);
*/
        }
        // end undo-redo operation
        net->getViewNet()->getUndoList()->p_end();
    }
}


void
GNERouteHandler::transformToTrip(GNEVehicle* originalVehicle) {
    // get original vehicle tag
    SumoXMLTag tag = originalVehicle->getTagProperty().getTag();
    // get pointer to net
    GNENet* net = originalVehicle->getNet();
    // obtain vehicle parameters
    SUMOVehicleParameter vehicleParameters = *originalVehicle;
    // get route
    GNEDemandElement* route = nullptr;
    // declare edges
    std::vector<GNEEdge*> edges;
    // obtain edges depending of tag
    if ((tag == SUMO_TAG_VEHICLE) || (tag == GNE_TAG_FLOW_ROUTE)) {
        // set route
        route = originalVehicle->getParentDemandElements().back();
        // get route edges
        edges = route->getParentEdges();
    } else if ((tag == GNE_TAG_VEHICLE_WITHROUTE) || (tag == GNE_TAG_FLOW_WITHROUTE)) {
        // get embedded route edges
        edges = originalVehicle->getChildDemandElements().front()->getParentEdges();
    } else if ((tag == SUMO_TAG_TRIP) || (tag == SUMO_TAG_FLOW)) {
        // just take parent edges (from and to)
        edges = originalVehicle->getParentEdges();
    }
    // only continue if edges are valid
    if (edges.size() < 2) {
        // declare header
        const std::string header = "Problem transforming to vehicle";
        // declare message
        const std::string message = "Vehicle cannot be transformed. Invalid number of edges";
        // write warning
        WRITE_DEBUG("Opened FXMessageBox " + header);
        // open message box
        FXMessageBox::warning(originalVehicle->getNet()->getViewNet()->getApp(), MBOX_OK, header.c_str(), "%s", message.c_str());
        // write warning if netedit is running in testing mode
        WRITE_DEBUG("Closed FXMessageBox " + header);
    } else {
        // begin undo-redo operation
        net->getViewNet()->getUndoList()->p_begin("transform " + originalVehicle->getTagStr() + " to " + toString(SUMO_TAG_TRIP));
        // first delete vehicle
        net->deleteDemandElement(originalVehicle, net->getViewNet()->getUndoList());
        // check if route has to be deleted
        if (route && route->getChildDemandElements().empty()) {
            net->deleteDemandElement(route, net->getViewNet()->getUndoList());
        }
        // change tag in vehicle parameters
        vehicleParameters.tag = SUMO_TAG_TRIP;
        // create trip
/*
        buildTrip(net, true, vehicleParameters, edges.front(), edges.back(), {});
*/
        // end undo-redo operation
        net->getViewNet()->getUndoList()->p_end();
    }
}


void
GNERouteHandler::transformToFlow(GNEVehicle* originalVehicle) {
    // get original vehicle tag
    SumoXMLTag tag = originalVehicle->getTagProperty().getTag();
    // get pointer to net
    GNENet* net = originalVehicle->getNet();
    // obtain vehicle parameters
    SUMOVehicleParameter vehicleParameters = *originalVehicle;
    // declare route
    GNEDemandElement* route = nullptr;
    // declare edges
    std::vector<GNEEdge*> edges;
    // obtain edges depending of tag
    if ((tag == SUMO_TAG_VEHICLE) || (tag == GNE_TAG_FLOW_ROUTE)) {
        // set route
        route = originalVehicle->getParentDemandElements().back();
        // get route edges
        edges = route->getParentEdges();
    } else if ((tag == GNE_TAG_VEHICLE_WITHROUTE) || (tag == GNE_TAG_FLOW_WITHROUTE)) {
        // get embedded route edges
        edges = originalVehicle->getChildDemandElements().front()->getParentEdges();
    } else if ((tag == SUMO_TAG_TRIP) || (tag == SUMO_TAG_FLOW)) {
        // just take parent edges (from and to)
        edges = originalVehicle->getParentEdges();
    }
    // only continue if edges are valid
    if (edges.empty()) {
        // declare header
        const std::string header = "Problem transforming to vehicle";
        // declare message
        const std::string message = "Vehicle cannot be transformed. Invalid number of edges";
        // write warning
        WRITE_DEBUG("Opened FXMessageBox " + header);
        // open message box
        FXMessageBox::warning(originalVehicle->getNet()->getViewNet()->getApp(), MBOX_OK, header.c_str(), "%s", message.c_str());
        // write warning if netedit is running in testing mode
        WRITE_DEBUG("Closed FXMessageBox " + header);
    } else {
        // begin undo-redo operation
        net->getViewNet()->getUndoList()->p_begin("transform " + originalVehicle->getTagStr() + " to " + toString(SUMO_TAG_VEHICLE));
        // first delete vehicle
        net->deleteDemandElement(originalVehicle, net->getViewNet()->getUndoList());
        // check if route has to be deleted
        if (route && route->getChildDemandElements().empty()) {
            net->deleteDemandElement(route, net->getViewNet()->getUndoList());
        }
        // change depart
        if ((vehicleParameters.tag == SUMO_TAG_TRIP) || (vehicleParameters.tag == SUMO_TAG_VEHICLE) || (vehicleParameters.tag == GNE_TAG_VEHICLE_WITHROUTE)) {
            // set end
            vehicleParameters.repetitionEnd = vehicleParameters.depart + 3600;
            // set number
            vehicleParameters.repetitionNumber = 1800;
            vehicleParameters.parametersSet |= VEHPARS_NUMBER_SET;
            // unset parameters
            vehicleParameters.parametersSet &= ~VEHPARS_END_SET;
            vehicleParameters.parametersSet &= ~VEHPARS_VPH_SET;
            vehicleParameters.parametersSet &= ~VEHPARS_PERIOD_SET;
            vehicleParameters.parametersSet &= ~VEHPARS_PROB_SET;
        }
        // change tag in vehicle parameters
        vehicleParameters.tag = SUMO_TAG_FLOW;
        // create flow
/*
        buildFlow(net, true, vehicleParameters, edges.front(), edges.back(), {});
*/
        // end undo-redo operation
        net->getViewNet()->getUndoList()->p_end();
    }
}


void
GNERouteHandler::transformToPerson(GNEPerson* /*originalPerson*/) {
    //
}


void
GNERouteHandler::transformToPersonFlow(GNEPerson* /*originalPerson*/) {
    //
}


void
GNERouteHandler::transformToContainer(GNEContainer* /*originalContainer*/) {
    //
}


void
GNERouteHandler::transformToContainerFlow(GNEContainer* /*originalContainer*/) {
    //
}


void
GNERouteHandler::setFlowParameters(const SumoXMLAttr attribute, int& parameters) {
    // modify parametersSetCopy depending of given Flow attribute
    switch (attribute) {
        case SUMO_ATTR_END: {
            // give more priority to end
            parameters = VEHPARS_END_SET | VEHPARS_NUMBER_SET;
            break;
        }
        case SUMO_ATTR_NUMBER:
            parameters ^= VEHPARS_END_SET;
            parameters |= VEHPARS_NUMBER_SET;
            break;
        case SUMO_ATTR_VEHSPERHOUR:
        case SUMO_ATTR_PERSONSPERHOUR: {
            // give more priority to end
            if ((parameters & VEHPARS_END_SET) && (parameters & VEHPARS_NUMBER_SET)) {
                parameters = VEHPARS_END_SET;
            } else if (parameters & VEHPARS_END_SET) {
                parameters = VEHPARS_END_SET;
            } else if (parameters & VEHPARS_NUMBER_SET) {
                parameters = VEHPARS_NUMBER_SET;
            }
            // set VehsPerHour
            parameters |= VEHPARS_VPH_SET;
            break;
        }
        case SUMO_ATTR_PERIOD: {
            // give more priority to end
            if ((parameters & VEHPARS_END_SET) && (parameters & VEHPARS_NUMBER_SET)) {
                parameters = VEHPARS_END_SET;
            } else if (parameters & VEHPARS_END_SET) {
                parameters = VEHPARS_END_SET;
            } else if (parameters & VEHPARS_NUMBER_SET) {
                parameters = VEHPARS_NUMBER_SET;
            }
            // set period
            parameters |= VEHPARS_PERIOD_SET;
            break;
        }
        case SUMO_ATTR_PROB: {
            // give more priority to end
            if ((parameters & VEHPARS_END_SET) && (parameters & VEHPARS_NUMBER_SET)) {
                parameters = VEHPARS_END_SET;
            } else if (parameters & VEHPARS_END_SET) {
                parameters = VEHPARS_END_SET;
            } else if (parameters & VEHPARS_NUMBER_SET) {
                parameters = VEHPARS_NUMBER_SET;
            }
            // set probability
            parameters |= VEHPARS_PROB_SET;
            break;
        }
        default:
            break;
    }
}

// ===========================================================================
// protected
// ===========================================================================

GNEEdge*
GNERouteHandler::parseEdge(const SumoXMLTag tag, const std::string& edgeID) const {
    GNEEdge* edge = myNet->retrieveEdge(edgeID, false);
    // empty edges aren't allowed. If edge is empty, write error, clear edges and stop
    if (edge == nullptr) {
        WRITE_ERROR("Could not build " + toString(tag) + " in netedit; " +  toString(SUMO_TAG_EDGE) + " doesn't exist.");
    }
    return edge;
}


std::vector<GNEEdge*>
GNERouteHandler::parseEdges(const SumoXMLTag tag, const std::vector<std::string>& edgeIDs) const {
    std::vector<GNEEdge*> edges;
    for (const auto &edgeID : edgeIDs) {
        GNEEdge* edge = myNet->retrieveEdge(edgeID, false);
        // empty edges aren't allowed. If edge is empty, write error, clear edges and stop
        if (edge == nullptr) {
            WRITE_ERROR("Could not build " + toString(tag) + " in netedit; " +  toString(SUMO_TAG_EDGE) + " doesn't exist.");
            edges.clear();
            return edges;
        } else {
            edges.push_back(edge);
        }
    }
    return edges;
}

GNEDemandElement*
GNERouteHandler::getPersonParent(const CommonXMLStructure::SumoBaseObject* sumoBaseObject) const {
    // check that sumoBaseObject has parent
    if (sumoBaseObject->getParentSumoBaseObject() == nullptr) {
        return nullptr;
    }
    if ((sumoBaseObject->getParentSumoBaseObject()->getTag() != SUMO_TAG_PERSON) && 
        (sumoBaseObject->getParentSumoBaseObject()->getTag() != SUMO_TAG_PERSONFLOW)) {
        return nullptr;
    }
    // try it with person
    GNEDemandElement* personParent = myNet->retrieveDemandElement(SUMO_TAG_PERSON, sumoBaseObject->getParentSumoBaseObject()->getStringAttribute(SUMO_ATTR_ID), false);
    // if empty, try it with personFlow
    if (personParent == nullptr) {
        return myNet->retrieveDemandElement(SUMO_TAG_PERSONFLOW, sumoBaseObject->getParentSumoBaseObject()->getStringAttribute(SUMO_ATTR_ID), false);
    } else {
        return personParent;
    }
}

/****************************************************************************/
