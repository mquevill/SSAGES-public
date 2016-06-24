#include "Method.h"
#include "json/json.h"
#include "schema.h"
#include "../Validator/ObjectRequirement.h"
#include "../Validator/ArrayRequirement.h"
#include "../Drivers/DriverException.h"
#include "ElasticBand.h"
#include "FiniteTempString.h"
#include "Meta.h"
#include "Umbrella.h"
#include "ForwardFlux.h"
#include "GridTest.h"
#include "ABF.h"

using namespace Json;

namespace SSAGES
{
	Method* Method::BuildMethod(const Value &json, 
						boost::mpi::communicator& world, 
						boost::mpi::communicator& comm,
						const std::string& path)
	{
		ObjectRequirement validator;
		Value schema;
		Reader reader;

		Method* method = nullptr;

		// Random device for seed generation. 
		// std::random_device rd;
		// auto maxi = std::numeric_limits<int>::max();
		// auto seed = json.get("seed", rd() % maxi).asUInt();
		
		// Get method type. 
		std::string type = json.get("type", "none").asString();

		if(type == "Umbrella")
		{
			reader.parse(JsonSchema::UmbrellaMethod, schema);
			validator.Parse(schema, path);

			// Validate inputs.
			validator.Validate(json, path);
			if(validator.HasErrors())
				throw BuildException(validator.GetErrors());

			std::vector<double> ksprings;
			for(auto& s : json["ksprings"])
				ksprings.push_back(s.asDouble());

			std::vector<double> centers;
			for(auto& s : json["centers"])
				centers.push_back(s.asDouble());

			if(ksprings.size() != centers.size())
				throw BuildException({"Need to define a spring for every center or a center for every spring!"});

			auto freq = json.get("frequency", 1).asInt();

			auto* m = new Umbrella(world, comm, ksprings, centers, freq);

			method = static_cast<Method*>(m);
		}
		else if(type == "Metadynamics")
		{
			reader.parse(JsonSchema::MetadynamicsMethod, schema);
			validator.Parse(schema, path);

			// Validate inputs.
			validator.Validate(json, path);
			if(validator.HasErrors())
				throw BuildException(validator.GetErrors());

			std::vector<double> widths;
			for(auto& s : json["widths"])
				widths.push_back(s.asDouble());

			auto height = json.get("height", 1.0).asDouble();
			auto hillfreq = json.get("hill frequency", 1).asInt();
			auto freq = json.get("frequency", 1).asInt();			

			auto* m = new Meta(world, comm, height, widths, hillfreq, freq);

			method = static_cast<Method*>(m);
		}
		else if(type == "ElasticBand")
		{
			reader.parse(JsonSchema::ElasticBandMethod, schema);
			validator.Parse(schema, path);

			// Validate inputs.
			validator.Validate(json, path);
			if(validator.HasErrors())
				throw BuildException(validator.GetErrors());

			std::vector<double> ksprings;
			for(auto& s : json["ksprings"])
				ksprings.push_back(s.asDouble());

			std::vector<double> centers;
			for(auto& s : json["centers"])
				centers.push_back(s.asDouble());

			auto isteps = json.get("max iterations", 2000).asInt();
			auto eqsteps = json.get("equilibration steps", 20).asInt();
			auto evsteps = json.get("evolution steps", 20).asInt();
			auto nsamples = json.get("number samples", 20).asInt();
			auto stringspring = json.get("kstring", 10.0).asDouble();
			auto timestep = json.get("time step", 1.0).asDouble();			
			auto freq = json.get("frequency", 1).asInt();			

			auto* m = new ElasticBand(world, comm, isteps, eqsteps,
			 						evsteps, nsamples, centers, ksprings,
			 						stringspring, timestep, freq);

			method = static_cast<Method*>(m);
		}
		else if(type == "ABF")
		{
			reader.parse(JsonSchema::ABFMethod, schema);
			validator.Parse(schema, path);

			// Validate inputs.
			validator.Validate(json, path);
			if(validator.HasErrors())
				throw BuildException(validator.GetErrors());

			std::vector<double> minsCV;
			for(auto& mins : json["CV minimums"])
				minsCV.push_back(mins.asDouble());
			
			std::vector<double> maxsCV;
			for(auto& maxs : json["CV maximums"])
				maxsCV.push_back(maxs.asDouble());

			std::vector<double> binsCV;
			for(auto& bins : json["CV bins"])
				binsCV.push_back(bins.asDouble());

			std::vector<double> minsrestCV;
			for(auto& mins : json["CV restraint minimums"])
				minsrestCV.push_back(mins.asDouble());
			
			std::vector<double> maxsrestCV;
			for(auto& maxs : json["CV restraint maximums"])
				maxsrestCV.push_back(maxs.asDouble());

			std::vector<double> springkrestCV;
			for(auto& bins : json["CV restraint spring constants"])
				springkrestCV.push_back(bins.asDouble());

			std::vector<int> printdetails;
			for(auto& bins : json["Print details"])
				printdetails.push_back(bins.asDouble());

			if(printdetails.size()!=9)
				{
				std::cout << "Print details not provided, or entered incorrectly. Defaulting to presets.";
				std::vector<int> preset = {1000, 1, 1, 1, 1, 1, 1, 1, 1};
				printdetails = preset;
				}

			int FBackupInterv = json.get("Backup interval", 1000).asInt();

			double unitconv = json.get("Unit conversion", 0).asDouble();
		
			int Orthogonalization = json.get("Orthogonalization", 1).asInt();

			double timestep = json.get("timestep",2).asDouble();

			double min = json.get("minimum count",100).asDouble();

			std::vector<std::vector<double>> histdetails;
			std::vector<std::vector<double>> restraint;
			std::vector<double> temp1(3);
			std::vector<double> temp2(3);

			for(size_t i=0; i<minsCV.size(); ++i)
				{
				temp1 = {minsCV[i], maxsCV[i], binsCV[i]};
				temp2 = {minsrestCV[i], maxsrestCV[i], springkrestCV[i]};
				histdetails.push_back(temp1);
				restraint.push_back(temp2);
				}		
			
			auto freq = json.get("frequency", 1).asInt();

			std::string readF = json.get("F from file", " ").asString();

			auto* m = new ABF(world, comm, histdetails, restraint, timestep, min, readF, printdetails, FBackupInterv, unitconv, Orthogonalization, freq);

			method = static_cast<Method*>(m);
		}
		else if(type == "ForwardFlux")
		{
			reader.parse(JsonSchema::ForwardFluxMethod, schema);
			validator.Parse(schema, path);

			// Validate inputs.
			validator.Validate(json, path);
			if(validator.HasErrors())
				throw BuildException(validator.GetErrors());

			std::vector<std::vector<double> > centers;
			for(auto& s : json["centers"])
			{
				std::vector<double> temp;
				for(auto& c : s["center"])
					temp.push_back(c.asDouble());
				centers.push_back(temp);
			}

			auto libraryfile = json.get("library file", "none").asString();
			auto resultsfile = json.get("results file", "none").asString();
			auto restartfile = json.get("restart file", "none").asString();
			auto newrun = json.get("new run",true).asBool();
			auto currentinterface = json.get("starting interface",0).asInt(); 
			auto genconfig = json.get("generate configs",1).asInt();
			auto shots = json.get("shots",1).asInt();
			auto freq = json.get("frequency", 1).asInt();

			auto* m = new ForwardFlux(world, comm, libraryfile, resultsfile, 
				restartfile, currentinterface, centers, newrun,
				genconfig, shots, freq);

			method = static_cast<Method*>(m);
		}
		else if(type == "FiniteTemperatureString")
		{
			reader.parse(JsonSchema::FTSMethod, schema);
			validator.Parse(schema, path);

			// Validate inputs.
			validator.Validate(json, path);
			if(validator.HasErrors())
				throw BuildException(validator.GetErrors());

			std::vector<double> centers;
			for(auto& s : json["centers"])
				centers.push_back(s.asDouble());

			auto isteps = json.get("block iterations", 2000).asInt();
			auto kappa = json.get("kappa", 0.1).asDouble();
			auto tau = json.get("time step", 0.1).asDouble();			
			auto freq = json.get("frequency", 1).asInt();			

			//Todo: Fix how NumNodes is determined! Currently incorrect
			int NumNodes = comm.size();
			auto* m = new FiniteTempString(world, comm, isteps, 
									centers, NumNodes, kappa,
			 						tau, freq);

			method = static_cast<Method*>(m);
		}
		else if(type == "GridTest")
		{
			auto* m = new GridTest(world, comm, 1);
			method = static_cast<Method*>(m);
		}
		else
		{
			throw BuildException({path + ": Unknown method type specified."});
		}

		method->_grid = nullptr;
		return method;
	}
}
