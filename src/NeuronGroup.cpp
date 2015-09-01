/* 
* Copyright 2014-2015 Friedemann Zenke
*
* This file is part of Auryn, a simulation package for plastic
* spiking neural networks.
* 
* Auryn is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* Auryn is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with Auryn.  If not, see <http://www.gnu.org/licenses/>.
*
* If you are using Auryn or parts of it for your work please cite:
* Zenke, F. and Gerstner, W., 2014. Limits to high-speed simulations 
* of spiking neural networks using general-purpose computers. 
* Front Neuroinform 8, 76. doi: 10.3389/fninf.2014.00076
*/

#include "NeuronGroup.h"

NeuronGroup::NeuronGroup(NeuronID n, double loadmultiplier, NeuronID total ) : SpikingGroup(n, loadmultiplier, total )
{
	if ( evolve_locally() ) init();
}

void NeuronGroup::init()
{  		group_name = "NeuronGroup";

		// stringstream oss;
		// oss << description_string << " init";
		// logger->msg(oss.str(),VERBOSE);

		mem = get_state_vector("mem");
		thr = get_state_vector("thr");
		g_ampa = get_state_vector("g_ampa");
		g_gaba = get_state_vector("g_gaba");
		g_cursyn = get_state_vector("g_cursyn");
		g_nmda = get_state_vector("g_nmda");

#ifndef CODE_ALIGNED_SSE_INSTRUCTIONS
		// checking via default if those arrays are aligned
		if ( auryn_AlignOffset( mem->size, mem->data, sizeof(float), 16) 
				|| auryn_AlignOffset( thr->size, thr->data, sizeof(float), 16) 
				|| auryn_AlignOffset( g_ampa->size, g_ampa->data, sizeof(float), 16) 
				|| auryn_AlignOffset( g_nmda->size, g_nmda->data, sizeof(float), 16) 
				|| auryn_AlignOffset( g_gaba->size, g_gaba->data, sizeof(float), 16)  
				|| auryn_AlignOffset( g_cursyn->size, g_cursyn->data, sizeof(float), 16) ) 
			throw AurynMemoryAlignmentException();
#endif
}


void NeuronGroup::free()
{
	for ( NeuronID i = 0 ; i < post_state_traces.size() ; ++i ) {
		stringstream oss;
		oss << get_name() 
			<< ":: Freeing state trace "
			<< post_state_traces_state_names[i]
			<< " at "
			<< i;
		logger->msg(oss.str() ,VERBOSE);
		delete post_state_traces[i];
	}
}


NeuronGroup::~NeuronGroup()
{
	if ( evolve_locally() ) free();
}

void NeuronGroup::print_val(auryn_vector_float * vec, NeuronID i, const char * name)
{
           printf ("%s%d= %g\n",name, i, auryn_vector_float_get (vec, i));
}

void NeuronGroup::print_vec(auryn_vector_float * vec, const char * name)
{
	for (NeuronID i = 0; i < get_rank_size(); i++)
         {
			 print_val(vec,i,name);
         }
}

void NeuronGroup::print_state(NeuronID id)
{
           printf ("%g %g %g %g %g\n",auryn_vector_float_get (mem, id), auryn_vector_float_get (g_ampa, id), auryn_vector_float_get (g_gaba, id) ,auryn_vector_float_get (g_nmda, id), auryn_vector_float_get (g_cursyn, id));
}

AurynState NeuronGroup::get_mem(NeuronID i)
{
	return get_val(mem,i);
}

auryn_vector_float * NeuronGroup::get_mem_ptr()
{
	return mem;
}

void NeuronGroup::set_mem(NeuronID i, AurynState val)
{
	set_val(mem,i,val);
}

void NeuronGroup::set_state(string name, NeuronID i, AurynState val)
{
	auryn_vector_float * tmp = get_state_vector(name);
	if ( tmp )
		set_val(tmp,i,val);
}

void NeuronGroup::set_state(string name, AurynState val)
{
	auryn_vector_float * tmp = get_state_vector(name);
	if ( tmp ) 
		for ( NeuronID i = 0 ; i < get_rank_size() ; ++i )
			set_val(tmp,i,val);
}




AurynState NeuronGroup::get_ampa(NeuronID i)
{
	return get_val(g_ampa,i);
}

AurynState NeuronGroup::get_cursyn(NeuronID i)
{
	return get_val(g_cursyn,i);
}

auryn_vector_float * NeuronGroup::get_ampa_ptr()
{
	return g_ampa;
}

auryn_vector_float * NeuronGroup::get_cursyn_ptr()
{
	return g_cursyn;
}

AurynState NeuronGroup::get_gaba(NeuronID i)
{
	return get_val(g_gaba,i);
}

auryn_vector_float * NeuronGroup::get_gaba_ptr()
{
	return g_gaba;
}

AurynState NeuronGroup::get_nmda(NeuronID i)
{
	return get_val(g_nmda,i);
}

auryn_vector_float * NeuronGroup::get_nmda_ptr()
{
	return g_nmda;
}

void NeuronGroup::set_ampa(NeuronID i, AurynState val)
{
	set_val(g_ampa,i,val);
}

void NeuronGroup::set_cursyn(NeuronID i, AurynState val)
{
	set_val(g_cursyn,i,val);
}

void NeuronGroup::set_gaba(NeuronID i, AurynState val)
{
	set_val(g_gaba,i,val);
}

void NeuronGroup::set_nmda(NeuronID i, AurynState val)
{
	set_val(g_nmda,i,val);
}

void NeuronGroup::random_mem(AurynState mean, AurynState sigma)
{
	randomize_state_vector_gauss("mem",mean,sigma,42);
	init_state();
}

void NeuronGroup::random_uniform_mem(AurynState lo, AurynState hi)
{
	boost::mt19937 ng_gen(42+communicator->rank()); // produces same series every time 
	boost::uniform_01<boost::mt19937> die = boost::uniform_01<boost::mt19937> (ng_gen);
	AurynState rv;

	for ( AurynLong i = 0 ; i<get_rank_size() ; ++i ) {
		rv = lo+die()*(hi-lo);
		set_mem(i,rv);
	}

	init_state();
}

void NeuronGroup::random_nmda(AurynState mean, AurynState sigma)
{
	boost::mt19937 ng_gen(53+communicator->rank()); // produces same series every time 
	boost::normal_distribution<> dist((double)mean, (double)sigma);
	boost::variate_generator<boost::mt19937&, boost::normal_distribution<> > die(ng_gen, dist);
	AurynState rv;

	for ( AurynLong i = 0 ; i<get_rank_size() ; ++i ) {
		rv = die();
		set_nmda(i,rv);
	}

	init_state();
}

void NeuronGroup::print_mem()
{
	print_vec(mem,"mem");
}

void NeuronGroup::print_ampa()
{
	print_vec(g_ampa,"g_ampa");
}

void NeuronGroup::print_gaba()
{
	print_vec(g_gaba,"g_gaba");
}

void NeuronGroup::print_nmda()
{
	print_vec(g_nmda,"g_nmda");
}

void NeuronGroup::print_cursyn()
{
	print_vec(g_nmda,"g_cursyn");
}

void NeuronGroup::safe_tadd(NeuronID id, AurynWeight amount, TransmitterType t)
{
	if (localrank(id))
		tadd(id, amount, t);
}

void NeuronGroup::init_state()
{

}






void NeuronGroup::set_val(auryn_vector_float * vec, NeuronID i, AurynState val)
{
    auryn_vector_float_set (vec, i, val);
}

void NeuronGroup::add_val(auryn_vector_float * vec, NeuronID i, AurynState val)
{
	// auryn_vector_float_set(vec,i,auryn_vector_float_get(vec,i)+val);
	vec->data[i] += val;
}

void NeuronGroup::clip_val(auryn_vector_float * vec, NeuronID i, AurynState max)
{
	if ( auryn_vector_float_get(vec,i) > max)
		auryn_vector_float_set(vec,i,max);
}

AurynState NeuronGroup::get_val(auryn_vector_float * vec, NeuronID i)
{
	return auryn_vector_float_get (vec, i);
}

void NeuronGroup::tadd(NeuronID id, AurynWeight amount, TransmitterType t)
{
	NeuronID localid = global2rank(id);
	if (t==GLUT) {
		add_val(g_ampa,localid,amount);
	} else {
		add_val(g_gaba,localid,amount);
	}
}


EulerTrace * NeuronGroup::get_post_state_trace( AurynFloat tau, string state_name, AurynFloat b ) 
{
	// first let's check if a state with that name exists
	if ( find_state_vector( state_name ) == NULL ) {
		logger->msg("A state vector with this name "+state_name+" does not exist");
		throw AurynStateVectorException();
	} // good to go

	for ( NeuronID i = 0 ; i < post_state_traces.size() ; i++ ) {
		if ( post_state_traces[i]->get_tau() == tau 
				&& post_state_traces_spike_biases[i] == b
				&& post_state_traces_state_names[i] == state_name ) {
			stringstream oss;
			oss << get_name() 
				<< ":: Sharing post state trace for "
				<< state_name 
				<< " with " 
				<< tau 
				<< "s timeconstant." ;
			logger->msg(oss.str(),VERBOSE);
			return post_state_traces[i];
		}
	}

	// trace does not exist yet, so we are creating 
	// it and do the book keeping
	logger->msg("Creating new post state trace",VERBOSE);
	EulerTrace * tmp = new EulerTrace(get_post_size(),tau);
	tmp->set_target(get_state_vector(state_name));
	post_state_traces.push_back(tmp);
	post_state_traces_spike_biases.push_back(b);
	post_state_traces_state_names.push_back(state_name);
	return tmp;
}


void NeuronGroup::evolve_traces()
{
	SpikingGroup::evolve_traces();

	for ( NeuronID i = 0 ; i < post_state_traces.size() ; i++ ) {

		// spike triggered component
		for (SpikeContainer::const_iterator spike = get_spikes_immediate()->begin() ; 
				spike != get_spikes_immediate()->end() ; 
				++spike ) {
			NeuronID translated_spike = global2rank(*spike); // only to be used for post traces
			post_state_traces[i]->add(translated_spike, post_state_traces_spike_biases[i]);
		}

		// follow the target vector (instead of evolve)
		post_state_traces[i]->follow();
	}
}

void NeuronGroup::virtual_serialize(boost::archive::binary_oarchive & ar, const unsigned int version ) 
{
	SpikingGroup::virtual_serialize(ar, version);

	logger->msg("NeuronGroup:: serializing state traces",VERBOSE);
	for ( NeuronID i = 0 ; i < post_state_traces.size() ; ++i )
		ar & *(post_state_traces[i]);
}

void NeuronGroup::virtual_serialize(boost::archive::binary_iarchive & ar, const unsigned int version ) 
{
	SpikingGroup::virtual_serialize(ar, version);

	logger->msg("NeuronGroup:: loading state traces",VERBOSE);
	for ( NeuronID i = 0 ; i < post_state_traces.size() ; ++i )
		ar & *(post_state_traces[i]);
}
