/**
 *  The MIT License (MIT)
 *
 *  Copyright (c) 2015 Kyle Wray, University of Massachusetts
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy of
 *  this software and associated documentation files (the "Software"), to deal in
 *  the Software without restriction, including without limitation the rights to
 *  use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 *  the Software, and to permit persons to whom the Software is furnished to do so,
 *  subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in all
 *  copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 *  FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 *  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 *  IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 *  CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#ifndef LOSMPOMDP_H
#define LOSMPOMDP_H

#include "lpomdp.h"

#include "../../librbr/librbr/include/core/states/state.h"
#include "../../librbr/librbr/include/core/actions/action.h"
#include "../../librbr/librbr/include/core/observations/observation.h"

#include "../../librbr/librbr/include/core/policy/policy_alpha_vectors.h"

#include "../../losm/losm/include/losm.h"

#include "losm_state.h"

#include <vector>
#include <unordered_map>

#define TO_SECONDS 60.0

#define NUM_TIREDNESS_LEVELS 2

#define INTERSECTION_WAIT_TIME_IN_SECONDS 0.1
#define AUTONOMY_SPEED_LIMIT_THRESHOLD 30.0
#define AUTONOMY_SPEED_LIMIT_FACTOR 0.9

// Boston (Commons)
//#define INITIAL_NODE_1 61362488
//#define INITIAL_NODE_2 61362484
//#define GOAL_NODE_1 61356537
//#define GOAL_NODE_2 61515075

// Boston (Small)
//#define INITIAL_NODE_1 61371580
//#define INITIAL_NODE_2 61341710
//#define GOAL_NODE_1 61505151
//#define GOAL_NODE_2 61356471

// Riverside Park
//#define GOAL_NODE_1 2329911911
//#define GOAL_NODE_2 2329911973
//#define INITIAL_NODE_1 2141026506
//#define INITIAL_NODE_2 42453220

// Amherst (Small)
//#define GOAL_NODE_1 66757197
//#define GOAL_NODE_2 66703862

/**
 * A LPOMDP for a LOSM object.
 */
class LOSMPOMDP : public LPOMDP {
public:
	/**
	 * The default constructor for the LOSMPOMDP class. It requires the specification of the three
	 * LOSM files to use.
	 * @param	nodesFilename 		The name of the LOSM nodes file to load.
	 * @param	edgesFilename 		The name of the LOSM edges file to load.
	 * @param	landmarksFilename	The name of the LOSM landmarks file to load.
	 * @param	goal1				The first goal node's UID.
	 * @param	goal2				The second goal node's UID.
	 */
	LOSMPOMDP(std::string nodesFilename, std::string edgesFilename, std::string landmarksFilename,
			std::string goal1, std::string goal2);

	/**
	 * A deconstructor for the LOSMPOMDP class.
	 */
	virtual ~LOSMPOMDP();

	/**
	 * Set the delta values.
	 * @param	d1	The first delta.
	 * @param	d2	The second delta.
	 */
	void set_slack(float d1, float d2);

	/**
	 * Save a PolicyAlphaVectors object to the custom format required by the visualizer.
	 * This assumes infinite horizon, and a particular belief probability over attentive / tired.
	 * @param	policy			The policy represented as alpha-vectors, one for each value
	 * 							function, mapping beliefs to actions.
	 * @param	k				The number of value functions.
	 * @param	filename		The name of the file to save.
	 * @return	Returns true if an error arose, and false otherwise.
	 */
	bool save_policy(PolicyAlphaVectors **policy, unsigned int k, std::string filename);

	/**
	 * Save a PolicyAlphaVectors object to the custom format required by the visualizer.
	 * This assumes infinite horizon, and a particular belief probability over attentive / tired.
	 * @param	policy			The policy represented as alpha-vectors, one for each value
	 * 							function, mapping beliefs to actions.
	 * @param	k				The number of value functions.
	 * @param	tirednessBelief	The belief value for if the driver is fatigued or not. This
	 * 							adjusts the output of saving, instead referring to tired as
	 * 							the policy for tirednessBelief, and attentive as the policy
	 * 							for 1 - tirednessBelief.
	 * @param	filename		The name of the file to save.
	 * @return	Returns true if an error arose, and false otherwise.
	 */
	bool save_policy(PolicyAlphaVectors **policy, unsigned int k, double tirednessBelief, std::string filename);

	/**
	 * Get the initial state, as defined by the constructor's two UIDs.
	 * @param	initial1		The first initial node's UID.
	 * @param	initial2		The second initial node's UID.
	 * @throw	CoreException	Could not find the initial state, or the UIDs were not long ints.
	 * @return	The initial state.
	 */
	LOSMState *get_initial_state(std::string initial1, std::string initial2);

	/**
	 * Set the weights for the factored weighted rewards.
	 * @param	weights		The new weight vector.
	 */
	void set_rewards_weights(const std::vector<double> &weights);

	/**
	 * Get the weights for the factored weighted rewards.
	 * @return	The weight vector.
	 */
	const std::vector<double> &get_rewards_weights() const;

	/**
	 * Get the set of goal states.
	 * @return	The set of goal states.
	 */
	const std::vector<LOSMState *> &get_goal_states() const;

	/**
	 * Get the set of tiredness states vector.
	 * @return	The set of tiredness states vector.
	 */
	const std::vector<std::vector<LOSMState *> > &get_tiredness_states() const;

private:
	/**
	 * Create the helper hash function for edges.
	 * @param	losm	The Light-OSM object.
	 */
	void create_edges_hash(LOSM *losm);

	/**
	 * Create the LPOMDP's states from the LOSM object provided.
	 * @param	losm	The Light-OSM object.
	 */
	void create_states(LOSM *losm);

	/**
	 * Create the LPOMDP's actions from the LOSM object provided.
	 * @param	losm	The Light-OSM object.
	 */
	void create_actions(LOSM *losm);

	/**
	 * Create the LPOMDP's observations from the LOSM object provided.
	 * @param	losm	The Light-OSM object.
	 */
	void create_observations(LOSM *losm);

	/**
	 * Create the LPOMDP's state transitions from the LOSM object provided.
	 * @param	losm	The Light-OSM object.
	 */
	void create_state_transitions(LOSM *losm);

	/**
	 * Create the LPOMDP's observation transitions from the LOSM object provided.
	 * @param	losm	The Light-OSM object.
	 */
	void create_observation_transitions(LOSM *losm);

	/**
	 * Create the LPOMDP's rewards from the LOSM object provided.
	 * @param	losm	The Light-OSM object.
	 */
	void create_rewards(LOSM *losm);

	/**
	 * Create the LPOMDP's initial and horizon objects from the LOSM object provided.
	 * @param 	losm	The Light-OSM object.
	 */
	void create_misc(LOSM *losm);

	/**
	 * Map directed path on the graph to merge nodes between intersections.
	 * @param	losm			The LOSM object.
	 * @param	current			The current node.
	 * @param	previous		The previous node.
	 * @param	distance		The distance (mi) traveled so far.
	 * @param	speedLimit		The weighted average speed limit (mi / h) traveled so far.
	 * @param	result			The resultant node of this computation.
	 * @param	resultStep		The previous node away from the resultant node of this computation.
	 */
	void map_directed_path(LOSM *losm, const LOSMNode *current, const LOSMNode *previous,
			float &distance, float &speedLimit,
			const LOSMNode *&result, const LOSMNode *&resultStep);

	/**
	 * Distance from a point to a line formed by two points.
	 * @param	x0	The point's x-axis.
	 * @param	y0	The point's y-axis.
	 * @param	x1	The first point on the line's x-axis.
	 * @param	y1	The first point on the line's y-axis.
	 * @param	x2	The second point on the line's x-axis.
	 * @param	y2	The second point on the line's y-axis.
	 * @return	The distance from the point to the line formed by the two points.
	 */
	float point_to_line_distance(float x0, float y0, float x1, float y1, float x2, float y2);

	/**
	 * The LOSM object which holds the graph structure. All the nodes are managed by this object.
	 */
	LOSM *losm;

	/**
	 * A helper hash which maps the combination of two LOSMNode pointers to an edge.
	 * This is used to quickly reference the edges from the combination of LOSMNode UIDs.
	 */
	std::unordered_map<unsigned int, std::unordered_map<unsigned int, const LOSMEdge *> > edgeHash;

	/**
	 * A map of the successor for taking an action, in which we ignore the tiredness value of
	 * the successor state. Used to save a policy for the visualizer.
	 */
	std::unordered_map<LOSMState *, std::unordered_map<unsigned int, LOSMState *> > successors;

	/**
	 * One of the two goal node's UID.
	 */
	unsigned long goalNodeUID1;

	/**
	 * One of the two goal node's UID.
	 */
	unsigned long goalNodeUID2;

	/**
	 * The set of goal states as part of the LPOMDP.
	 */
	std::vector<LOSMState *> goalStates;

	/**
	 * The set of pairs of states which are attentive / tired for each physical location and
	 * whether or not autonomy is enabled.
	 */
	std::vector<std::vector<LOSMState *> > tirednessStates;

};


#endif // LOSMPOMDP_H
