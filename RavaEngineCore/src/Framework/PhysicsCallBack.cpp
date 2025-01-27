#include "ravapch.h"

#include "Framework/PhysicsSystem.h"
#include "Framework/Scene.h"
#include "Framework/Entity.h"

namespace Rava {
void PhysicsErrorCallback::reportError(physx::PxErrorCode::Enum code, const char* msg, const char* file, int line) {
	switch (code) {
		case physx::PxErrorCode::eNO_ERROR:
			ENGINE_INFO("File: {0}, Line: {1}, {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eINVALID_PARAMETER:
			ENGINE_ERROR("File: {0}, Line: {1}, Invlid parameter: {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eINVALID_OPERATION:
			ENGINE_ERROR("File: {0}, Line: {1}, Invalid operation: {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eOUT_OF_MEMORY:
			ENGINE_ERROR("File: {0}, Line: {1}, Out of memory: {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eDEBUG_INFO:
			ENGINE_INFO("File: {0}, Line: {1}, {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eDEBUG_WARNING:
			ENGINE_WARN("File: {0}, Line: {1}, {2}", file, line, msg);
			break;
		case physx::PxErrorCode::ePERF_WARNING:
			ENGINE_WARN("File: {0}, Line: {1}, Performance: {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eABORT:
			ENGINE_ERROR("File: {0}, Line: {1}, Abort: {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eINTERNAL_ERROR:
			ENGINE_ERROR("File: {0}, Line: {1}, Internal error: {2}", file, line, msg);
			break;
		case physx::PxErrorCode::eMASK_ALL:
			ENGINE_ERROR("File: {0}, Line: {1}, Unknown error: {2}", file, line, msg);
			break;
	}
}

void PhysicsEventCallback::onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) {
	for (physx::PxU32 i = 0; i < count; ++i) {
		auto& pair = pairs[i];

		if (pair.flags & (physx::PxTriggerPairFlag::eREMOVED_SHAPE_TRIGGER | physx::PxTriggerPairFlag::eREMOVED_SHAPE_OTHER)) {
			continue;
		}

		auto entityA = reinterpret_cast<Entity*>(pair.triggerActor->userData);
		auto entityB = reinterpret_cast<Entity*>(pair.otherActor->userData);

		if (entityA && entityB) {
			if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) {
				entityA->OnTriggerEnter(entityB);
				entityB->OnTriggerEnter(entityA);
			} else if (pair.status & physx::PxPairFlag::eNOTIFY_TOUCH_LOST) {
				entityA->OnTriggerExit(entityB);
				entityB->OnTriggerExit(entityA);
			}
		}
	}
}

void PhysicsEventCallback::onContact(
	const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs
) {
	for (physx::PxU32 i = 0; i < nbPairs; ++i) {
		auto& pair = pairs[i];

		auto entityA = reinterpret_cast<Entity*>(pairHeader.actors[0]->userData);
		auto entityB = reinterpret_cast<Entity*>(pairHeader.actors[1]->userData);

		if (entityA && entityB) {
			if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_FOUND) {
				entityA->OnContactEnter(entityB);
				entityB->OnContactEnter(entityA);
			} else if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_PERSISTS) {
				entityA->OnContactStay(entityB);
				entityB->OnContactStay(entityA);
			} else if (pair.events & physx::PxPairFlag::eNOTIFY_TOUCH_LOST) {
				entityA->OnContactExit(entityB);
				entityB->OnContactExit(entityA);
			}
		}
	}
}
}  // namespace Rava