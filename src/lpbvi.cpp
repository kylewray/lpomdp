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


#include <unistd.h>

#include "../include/lpbvi.h"

#include "../../librbr/librbr/include/pomdp/pomdp_utilities.h"

#include "../../librbr/librbr/include/management/conversion.h"

#include "../../librbr/librbr/include/core/state_transitions/state_transitions_array.h"
#include "../../librbr/librbr/include/core/rewards/sas_rewards_array.h"

#include "../../librbr/librbr/include/core/core_exception.h"
#include "../../librbr/librbr/include/core/states/state_exception.h"
#include "../../librbr/librbr/include/core/actions/action_exception.h"
#include "../../librbr/librbr/include/core/observations/observation_exception.h"
#include "../../librbr/librbr/include/core/state_transitions/state_transition_exception.h"
#include "../../librbr/librbr/include/core/observation_transitions/observation_transition_exception.h"
#include "../../librbr/librbr/include/core/rewards/reward_exception.h"
#include "../../librbr/librbr/include/core/policy/policy_exception.h"

#include "../../librbr/librbr/include/core/actions/action_utilities.h"

#include <iostream>

#include <math.h>
#include <vector>
#include <unordered_map>
#include <algorithm>

LPBVI::LPBVI() : POMDPPBVI()
{ }

LPBVI::LPBVI(POMDPPBVIExpansionRule expansionRule, unsigned int updateIterations,
		unsigned int expansionIterations) : POMDPPBVI(expansionRule, updateIterations, expansionIterations)
{ }

LPBVI::~LPBVI()
{ }

void LPBVI::compute_num_update_iterations(POMDP *pomdp, double epsilon)
{
	// This must be an LPOMDP.
	LPOMDP *lpomdp = dynamic_cast<LPOMDP *>(pomdp);
	if (lpomdp == nullptr) {
		throw CoreException();
	}

	Horizon *h = lpomdp->get_horizon();
	if (h == nullptr) {
		throw CoreException();
	}

	FactoredRewards *R = dynamic_cast<FactoredRewards *>(lpomdp->get_rewards());

	updates = 0;

	for (int i = 0; i < (int)R->get_num_rewards(); i++) {
		// Attempt to convert the rewards object into SARewards.
		SARewards *Ri = dynamic_cast<SARewards *>(lpomdp->get_rewards());
		if (Ri == nullptr) {
			throw RewardException();
		}

		// Make sure we do not take the log of 0.
		double Rmin = Ri->get_min();
		double Rmax = Ri->get_max();
		if (Rmax - Rmin < 0.000001) {
			Rmax = Rmin + 0.000001;
		}

		updates = (unsigned int)std::max((double)updates, (double)((log(epsilon) - log(Rmax - Rmin)) / log(h->get_discount_factor())));
	}
}

PolicyAlphaVectors *LPBVI::solve(POMDP *pomdp)
{
	throw CoreException();
}

PolicyAlphaVectors **LPBVI::solve(LPOMDP *lpomdp)
{
	// Handle the trivial case.
	if (lpomdp == nullptr) {
		return nullptr;
	}

	// Attempt to convert the states object into FiniteStates.
	StatesMap *S = dynamic_cast<StatesMap *>(lpomdp->get_states());
	if (S == nullptr) {
		throw StateException();
	}

	// Attempt to convert the actions object into FiniteActions.
	ActionsMap *A = dynamic_cast<ActionsMap *>(lpomdp->get_actions());
	if (A == nullptr) {
		throw ActionException();
	}

	// Attempt to convert the observations object into FiniteObservations.
	ObservationsMap *Z = dynamic_cast<ObservationsMap *>(lpomdp->get_observations());
	if (Z == nullptr) {
		throw ObservationException();
	}

	// Attempt to get the state transitions.
	StateTransitions *T = lpomdp->get_state_transitions();
	if (T == nullptr) {
		throw StateTransitionException();
	}

	// Attempt to get the observations transitions.
	ObservationTransitions *O = lpomdp->get_observation_transitions();
	if (O == nullptr) {
		throw ObservationTransitionException();
	}

	// Attempt to convert the rewards object into FactoredRewards. Also, ensure that the
	// type of each element is SASRewards.
	FactoredRewards *R = dynamic_cast<FactoredRewards *>(lpomdp->get_rewards());
	if (R == nullptr) {
		throw RewardException();
	}
	/*
	for (int i = 0; i < R->get_num_rewards(); i++) {
		SARewards *Ri = dynamic_cast<SARewards *>(R->get(i));
		if (Ri == nullptr) {
			throw RewardException();
		}
	}
	*/

	// Handle the other trivial case in which the slack variables were incorrectly defined.
	if (lpomdp->get_slack().size() != R->get_num_rewards()) {
		throw RewardException();
	}
	for (int i = 0; i < (int)lpomdp->get_slack().size(); i++) {
		if (lpomdp->get_slack().at(i) < 0.0) {
			throw RewardException();
		}
	}

	// Obtain the horizon and return the correct value iteration.
	Horizon *h = lpomdp->get_horizon();
	if (h->is_finite()) {
		throw CoreException();
	}

	return solve_infinite_horizon(S, A, Z, T, O, R, h, lpomdp->get_slack());
}

PolicyAlphaVectors **LPBVI::solve_infinite_horizon(StatesMap *S, ActionsMap *A,
		ObservationsMap *Z, StateTransitions *T, ObservationTransitions *O,
		FactoredRewards *R, Horizon *h, std::vector<float> &delta)
{
	// The final set of alpha vectors.
	PolicyAlphaVectors **policy = new PolicyAlphaVectors*[R->get_num_rewards()];
	for (int i = 0; i < (int)R->get_num_rewards(); i++) {
		policy[i] = new PolicyAlphaVectors(h->get_horizon());
	}

	// Initialize the set of belief points to be the initial set. This must be a copy, since memory is managed
	// for both objects independently.
	for (BeliefState *b : initialB) {
		B.push_back(new BeliefState(*b));
	}

	// Perform a predefined number of expansions. Each update adds more belief points to the set B.
	for (unsigned int e = 0; e < expansions; e++) {

		// Create the set of actions available; it starts with all actions available.
		std::vector<Action *> Ai;
		for (auto a : *A) {
			Ai.push_back(resolve(a));
		}

		for (int i = 0; i < (int)R->get_num_rewards(); i++) {
			SARewards *Ri = dynamic_cast<SARewards *>(R->get(i));

			// Before anything, cache Gamma_{a, *} for all actions. This is used in every cross-sum computation.
			std::map<Action *, std::vector<PolicyAlphaVector *> > gammaAStar;
			for (Action *action : Ai) {
				gammaAStar[action].push_back(create_gamma_a_star(S, Z, T, O, Ri, action));
			}

			// Create the set of alpha vectors, which we call Gamma. As well as the previous Gamma set.
			std::vector<PolicyAlphaVector *> gamma[2];
			bool current = false;

			// Initialize the first set Gamma to be a set of zero alpha vectors.
			for (unsigned int i = 0; i < B.size(); i++) {
				PolicyAlphaVector *zeroAlphaVector = new PolicyAlphaVector();
				for (auto s : *S) {
					zeroAlphaVector->set(resolve(s), 0.0);
				}
				gamma[!current].push_back(zeroAlphaVector);
			}

			// Perform a predefined number of updates. Each update improves the value function estimate.
			for (unsigned int u = 0; u < updates; u++){
				// For each of the belief points, we must compute the optimal alpha vector.
				for (BeliefState *belief : B) {
					PolicyAlphaVector *maxAlphaB = nullptr;
					double maxAlphaDotBeta = 0.0;

					// Compute the optimal alpha vector for this belief state.
					for (Action *action : Ai) {

						// ****************************************************************************************************
						// ****************************************************************************************************
						// ****************************************************************************************************
						// ****************************************************************************************************
						// ****************************************************************************************************

						// HAS TO BE A SASORewards... You gotta fix this garbage... Just make it SARewards throughout the POMDP code.

						// ****************************************************************************************************
						// ****************************************************************************************************
						// ****************************************************************************************************
						// ****************************************************************************************************
						// ****************************************************************************************************


						PolicyAlphaVector *alphaBA = bellman_update_belief_state(S, Z, T, O, Ri, h,
								gammaAStar[action], gamma[!current], action, belief);

						double alphaDotBeta = alphaBA->compute_value(belief);
						if (maxAlphaB == nullptr || alphaDotBeta > maxAlphaDotBeta) {
							// This is the maximal alpha vector, so delete the old one.
							if (maxAlphaB != nullptr) {
								delete maxAlphaB;
							}
							maxAlphaB = alphaBA;
							maxAlphaDotBeta = alphaDotBeta;
						} else {
							// This was not the maximal alpha vector, so delete it.
							delete alphaBA;
						}
					}

					gamma[current].push_back(maxAlphaB);
				}

				// Prepare the next time step's gamma by clearing it. Remember again, we don't free the memory
				// because policy manages the previous time step's gamma (above). If this is the first horizon,
				// however, we actually do need to clear the set of zero alpha vectors.
				current = !current;
				for (PolicyAlphaVector *zeroAlphaVector : gamma[current]) {
					delete zeroAlphaVector;
				}
				gamma[current].clear();
			}

			// Set the current gamma to the policy object. Note: This transfers the responsibility of
			// memory management to the PolicyAlphaVectors object.
			policy[i]->set(gamma[!current]);

			// Free the memory of Gamma_{a, *}.
			for (Action *action : Ai) {
				for (PolicyAlphaVector *alphaVector : gammaAStar[action]) {
					delete alphaVector;
				}
				gammaAStar[action].clear();
			}
			gammaAStar.clear();
		}

		// Perform an expansion based on the rule the user wishes to use.
		switch (rule) {
		case POMDPPBVIExpansionRule::NONE:
			e = expansions; // Stop immediately if the user does not want to expand.
			break;
		case POMDPPBVIExpansionRule::RANDOM_BELIEF_SELECTION:
			expand_random_belief_selection(S);
			break;
		case POMDPPBVIExpansionRule::STOCHASTIC_SIMULATION_RANDOM_ACTION:
			expand_stochastic_simulation_random_actions(S, A, Z, T, O);
			break;
//		case POMDPPBVIExpansionRule::STOCHASTIC_SIMULATION_GREEDY_ACTION:
			// NOTE: This one is a bit harder, since gamma is inside another loop now, but this is outside
			// that loop... Just ignore it for now, and use the one below.
//			expand_stochastic_simulation_greedy_action(S, A, Z, T, O, gamma[!current]);
//			break;
		case POMDPPBVIExpansionRule::STOCHASTIC_SIMULATION_EXPLORATORY_ACTION:
			expand_stochastic_simulation_exploratory_action(S, A, Z, T, O);
			break;
		case POMDPPBVIExpansionRule::GREEDY_ERROR_REDUCTION:
			expand_greedy_error_reduction();
			break;
		default:
			throw PolicyException();
			break;
		};
	}

	return policy;
}
