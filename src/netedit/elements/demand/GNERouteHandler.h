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
/// @file    GNERouteHandler.h
/// @author  Pablo Alvarez Lopez
/// @date    Jan 2019
///
// Builds demand objects for netedit
/****************************************************************************/
#pragma once
#include <config.h>

#include <netedit/frames/GNEFrameAttributesModuls.h>
#include <netedit/frames/GNEFrameModuls.h>
#include <utils/common/SUMOVehicleClass.h>
#include <utils/vehicle/SUMORouteHandler.h>
#include <utils/xml/SUMOSAXAttributes.h>
#include <utils/xml/SUMOSAXHandler.h>
#include <utils/xml/SUMOXMLDefinitions.h>
#include <utils/handlers/RouteHandler.h>


// ===========================================================================
// class declarations
// ===========================================================================

class GNEViewNet;
class GNEEdge;
class GNETAZ;
class GNEDemandElement;
class GNEVehicle;
class GNEPerson;
class GNEContainer;
class GNEUndoList;

// ===========================================================================
// class definitions
// ===========================================================================

/// @class GNERouteHandler
/// @brief Builds trigger objects for GNENet (busStops, chargingStations, detectors, etc..)
class GNERouteHandler : public RouteHandler {

public:
    /// @brief Constructor
    GNERouteHandler(const std::string& file, GNENet* net, bool undoDemandElements = true);

    /// @brief Destructor
    ~GNERouteHandler();

/**************************************/

    /// @name build functions
    /// @{

    /// @brief build vType
    void buildVType(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVTypeParameter& vTypeParameter,
                    const std::map<std::string, std::string> &parameters);

    /// @brief build vType distribution
    void buildVTypeDistribution(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &id);

    /// @brief build route
    void buildRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &id, SUMOVehicleClass vClass, 
                    const std::vector<std::string> &edges, const RGBColor &color, const int repeat, const SUMOTime cycleTime, 
                    const std::map<std::string, std::string> &parameters);

    /// @brief build route distribution
    void buildRouteDistribution(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &id);

    /// @brief build a vehicle over an existent route
    void buildVehicleOverRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters,
                                const std::map<std::string, std::string> &parameters);

    /// @brief build a flow over an existent route
    void buildFlowOverRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters,
                            const std::map<std::string, std::string> &parameters);

    /// @brief build vehicle with a embedded route
    void buildVehicleEmbeddedRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, SUMOVehicleParameter vehicleParameters, 
                                    const std::vector<std::string>& edges, const std::map<std::string, std::string> &parameters);

    /// @brief build flow with a embedded route
    void buildFlowEmbeddedRoute(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, SUMOVehicleParameter vehicleParameters, 
                                const std::vector<std::string>& edges, const std::map<std::string, std::string> &parameters);

    /// @brief build trip
    void buildTrip(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters, 
                   const std::string &fromEdge, const std::string &toEdge, const std::vector<std::string>& via,
                   const std::map<std::string, std::string> &parameters);

    /// @brief build flow
    void buildFlow(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& vehicleParameters, 
                   const std::string &fromEdge, const std::string &toEdge, const std::vector<std::string>& via,
                   const std::map<std::string, std::string> &parameters);

    /// @brief build stop
    void buildStop(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter::Stop& stopParameters);

    /// @brief build person
    void buildPerson(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& personParameters,
                     const std::map<std::string, std::string> &parameters);

    /// @brief build person flow
    void buildPersonFlow(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& personFlowParameters,
                         const std::map<std::string, std::string> &parameters);

    /// @brief build person trip
    void buildPersonTrip(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge,
                         const std::string &toBusStop, double arrivalPos, const std::vector<std::string>& types, const std::vector<std::string>& modes);

    /// @brief build walk
    void buildWalk(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge,
                   const std::string &toBusStop, const std::vector<std::string>& edges, const std::string &route, double arrivalPos);

    /// @brief build ride
    void buildRide(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge, 
                   const std::string &toBusStop, double arrivalPos, const std::vector<std::string>& lines);

    /// @brief build container
    void buildContainer(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& containerParameters,
                        const std::map<std::string, std::string> &parameters);

    /// @brief build container flow
    void buildContainerFlow(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const SUMOVehicleParameter& containerFlowParameters,
                            const std::map<std::string, std::string> &parameters);

    /// @brief build transport
    void buildTransport(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge,
                                const std::string &toBusStop, const std::vector<std::string>& lines, const double arrivalPos);

    /// @brief build tranship
    void buildTranship(const CommonXMLStructure::SumoBaseObject* sumoBaseObject, const std::string &fromEdge, const std::string &toEdge,
                       const std::string &toBusStop, const std::vector<std::string>& edges, const double speed, const double departPosition, 
                       const double arrivalPosition);
    /// @}

/**************************************/

    /// @brief check if there is already a vehicle (Vehicle, Trip, Flow or Flow) with the given ID
    static bool isVehicleIdDuplicated(GNENet* net, const std::string& id);

    /// @brief check if there is already a person (Person or PersonFlow) with the given ID
    static bool isPersonIdDuplicated(GNENet* net, const std::string& id);

    /// @brief check if there is already a container (Container or ContainerFlow) with the given ID
    static bool isContainerIdDuplicated(GNENet* net, const std::string& id);

    /// @name build personPlan
    /// @{
    /// @brief build person plan functions (used in Person / PersonPlan frames)
    static bool buildPersonPlan(SumoXMLTag tag, GNEDemandElement* personParent, GNEFrameAttributesModuls::AttributesCreator* personPlanAttributes, GNEFrameModuls::PathCreator* pathCreator);

    /// @brief build person trip
    static void buildPersonTrip(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge, GNEAdditional* toBusStop,
                                double arrivalPos, const std::vector<std::string>& types, const std::vector<std::string>& modes);

    /// @brief build walk
    static void buildWalk(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge,
                          GNEAdditional* toBusStop, const std::vector<GNEEdge*>& edges, GNEDemandElement* route, double arrivalPos);

    /// @brief build ride
    static void buildRide(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* fromEdge, GNEEdge* toEdge, GNEAdditional* toBusStop,
                          double arrivalPos, const std::vector<std::string>& lines);

    /// @brief build person stop
    static void buildStopPerson(GNENet* net, bool undoDemandElements, GNEDemandElement* personParent, GNEEdge* edge, GNEAdditional* busStop, const SUMOVehicleParameter::Stop& stopParameters);

    /// @}

    /// @name build container
    /// @{
    /// @brief build container
    static void buildContainer(GNENet* net, bool undoDemandElements, const SUMOVehicleParameter& containerParameters);

    /// @brief build container flow
    static void buildContainerFlow(GNENet* net, bool undoDemandElements, const SUMOVehicleParameter& containerFlowParameters);

    /// @}

    /// @name build containerPlan
    /// @{
    /// @brief build container plan functions (used in Container / ContainerPlan frames)
    static bool buildContainerPlan(SumoXMLTag tag, GNEDemandElement* containerParent, GNEFrameAttributesModuls::AttributesCreator* containerPlanAttributes, GNEFrameModuls::PathCreator* pathCreator);

    /// @brief build transport
    static void buildTransport(GNENet* net, bool undoDemandElements, GNEDemandElement* containerParent, GNEEdge* fromEdge, GNEEdge* toEdge,
                          GNEAdditional* toBusStop, const std::vector<std::string>& lines, const double arrivalPos);

    /// @brief build tranship
    static void buildTranship(GNENet* net, bool undoDemandElements, GNEDemandElement* containerParent, GNEEdge* fromEdge, GNEEdge* toEdge,
                          GNEAdditional* toBusStop, const std::vector<GNEEdge*>& edges, const double speed, const double departPosition, const double arrivalPosition);

    /// @brief build container stop
    static void buildStopContainer(GNENet* net, bool undoDemandElements, GNEDemandElement* containerParent, GNEEdge* edge, GNEAdditional* containerStop, const SUMOVehicleParameter::Stop& stopParameters);

    /// @}


    /// @brief transform vehicle functions
    /// @{

    /// @brief transform to vehicle over an existent route
    static void transformToVehicle(GNEVehicle* originalVehicle, bool createEmbeddedRoute);

    /// @brief transform routeFlow over an existent route
    static void transformToRouteFlow(GNEVehicle* originalVehicle, bool createEmbeddedRoute);

    /// @brief transform to trip
    static void transformToTrip(GNEVehicle* originalVehicle);

    /// @brief transform to flow
    static void transformToFlow(GNEVehicle* originalVehicle);

    /// @}

    /// @brief transform person functions
    /// @{

    /// @brief transform to vehicle over an existent route
    static void transformToPerson(GNEPerson* originalPerson);

    /// @brief transform routeFlow over an existent route
    static void transformToPersonFlow(GNEPerson* originalPerson);

    /// @}

    /// @brief transform container functions
    /// @{

    /// @brief transform to vehicle over an existent route
    static void transformToContainer(GNEContainer* originalContainer);

    /// @brief transform routeFlow over an existent route
    static void transformToContainerFlow(GNEContainer* originalContainer);

    /// @}

    /// @brief configure flow parameters
    static void setFlowParameters(const SumoXMLAttr attribute, int& parameters);

protected:
    /// @brief parse edge
    GNEEdge* parseEdge(const SumoXMLTag tag, const std::string& edgeID) const;

    /// @brief parse edges
    std::vector<GNEEdge*> parseEdges(const SumoXMLTag tag, const std::vector<std::string>& edgeIDs) const;

    /// @brief get person parent
    GNEDemandElement *getPersonParent(const CommonXMLStructure::SumoBaseObject* sumoBaseObject) const;

private:
    /// @brief pointer to GNENet
    GNENet* myNet;

    /// @brief flag to check if created demand elements must be undo and redo
    bool myUndoDemandElements;
};


