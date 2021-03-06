
/**
 *    Copyright (C) 2018-present MongoDB, Inc.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the Server Side Public License, version 1,
 *    as published by MongoDB, Inc.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    Server Side Public License for more details.
 *
 *    You should have received a copy of the Server Side Public License
 *    along with this program. If not, see
 *    <http://www.mongodb.com/licensing/server-side-public-license>.
 *
 *    As a special exception, the copyright holders give permission to link the
 *    code of portions of this program with the OpenSSL library under certain
 *    conditions as described in each individual source file and distribute
 *    linked combinations including the program with the OpenSSL library. You
 *    must comply with the Server Side Public License in all respects for
 *    all of the code used other than as permitted herein. If you modify file(s)
 *    with this exception, you may extend this exception to your version of the
 *    file(s), but you are not obligated to do so. If you do not wish to do so,
 *    delete this exception statement from your version. If you delete this
 *    exception statement from all source files in the program, then also delete
 *    it in the license file.
 */

#include "mongo/platform/basic.h"

#include "mongo/db/operation_context_session_mongod.h"

#include "mongo/db/transaction_coordinator_factory.h"
#include "mongo/db/transaction_participant.h"

namespace mongo {

OperationContextSessionMongod::OperationContextSessionMongod(OperationContext* opCtx,
                                                             bool shouldCheckOutSession,
                                                             boost::optional<bool> autocommit,
                                                             boost::optional<bool> startTransaction,
                                                             boost::optional<bool> coordinator)
    : _operationContextSession(opCtx, shouldCheckOutSession) {
    if (shouldCheckOutSession && !opCtx->getClient()->isInDirectClient()) {
        const auto txnParticipant = TransactionParticipant::get(opCtx);
        const auto clientTxnNumber = *opCtx->getTxnNumber();

        txnParticipant->refreshFromStorageIfNeeded(opCtx);
        txnParticipant->beginOrContinue(clientTxnNumber, autocommit, startTransaction);

        if (startTransaction && *startTransaction) {
            // If this shard has been selected as the coordinator, set up the coordinator state
            // to be ready to receive votes.
            if (coordinator && *coordinator) {
                createTransactionCoordinator(opCtx, clientTxnNumber);
            }
        }
    }
}

OperationContextSessionMongodWithoutRefresh::OperationContextSessionMongodWithoutRefresh(
    OperationContext* opCtx)
    : _operationContextSession(opCtx, true /* checkout */) {
    invariant(!opCtx->getClient()->isInDirectClient());
    const auto clientTxnNumber = *opCtx->getTxnNumber();

    const auto txnParticipant = TransactionParticipant::get(opCtx);
    txnParticipant->beginOrContinueTransactionUnconditionally(clientTxnNumber);
}

}  // namespace mongo
