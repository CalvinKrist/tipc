#include "FunctionGroup.h"

FunctionGroup::FunctionGroup(ASTFunction* base){
	associatedFunctions.emplace(base);
}
void FunctionGroup::AddCall(FunctionGroup* group){
	possibleCalls.emplace(group);
	group->callsiteFuncs.emplace(this);
}
void FunctionGroup::Union(FunctionGroup* group){
	group->alive = false;

	possibleCalls.insert(group->possibleCalls.begin(), group->possibleCalls.end());
	callsiteFuncs.insert(group->callsiteFuncs.begin(), group->callsiteFuncs.end());
	associatedFunctions.insert(group->associatedFunctions.begin(), group->associatedFunctions.end());

	possibleCalls.erase(group);
	possibleCalls.erase(this);
	callsiteFuncs.erase(group);
	callsiteFuncs.erase(this);
}
void FunctionGroup::Finalize(){
	for(auto it = possibleCalls.begin(); it != possibleCalls.end(); it++){
		if(!(*it)->alive){
			possibleCalls.erase(it);
		}
	}
	for(auto it = callsiteFuncs.begin(); it != callsiteFuncs.end(); it++){
		if(!(*it)->alive){
			callsiteFuncs.erase(it);
		}
	}
	callsiteFuncsDuplicate = callsiteFuncs;
}
bool FunctionGroup::IsTerminal() const{ return callsiteFuncs.size() == 0; }
void FunctionGroup::PruneCallsite(FunctionGroup* group){ callsiteFuncs.erase(group); }
void FunctionGroup::RestoreCallsites(){ callsiteFuncs = callsiteFuncsDuplicate; }
std::string FunctionGroup::GetName(){
	if(associatedFunctions.size() > 1){
		std::string name{ (*associatedFunctions.begin())->getName() };
		for(auto i = ++associatedFunctions.begin(); i != associatedFunctions.end(); i++){
			name += ", " + (*i)->getName();
		}
		return "(" + name + ")";
	} else{
		return (*associatedFunctions.begin())->getName();
	}
}
const std::set<FunctionGroup*>& FunctionGroup::GetCalls() const{ return possibleCalls; }
const std::set<ASTFunction*>& FunctionGroup::GetFuncs() const{ return associatedFunctions; }