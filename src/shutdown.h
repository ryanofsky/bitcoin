// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef BITCOIN_SHUTDOWN_H
#define BITCOIN_SHUTDOWN_H

/** Initialize shutdown state. This must be called before using either StartShutdown(),
 * AbortShutdown() or WaitForShutdown(). Calling ShutdownRequested() is always safe.
 */
bool InitShutdownState();

/** Request shutdown of the application. */
void StartShutdown(bool posix_signal = false);

/** Clear shutdown flag. Only use this during init (before calling WaitForShutdown in any
 * thread), or in the unit tests. Calling it in other circumstances will cause a race condition.
 */
void AbortShutdown();

/** Returns true if a shutdown is requested, false otherwise. */
bool ShutdownRequested();

/** Wait for StartShutdown to be called in any thread. This can only be used
 * from a single thread.
 */
void WaitForShutdown();

/** Asynchronously forward POSIX signals from internal socket to uiInterface.
 *
 * Only needed on POSIX platforms, but safe to call everywhere.
 *
 * HandleAsyncShutdown() is an alternative to calling WaitForShutdown(). If
 * application already has an idle thread not doing anything (like bitcoind), it
 * can use WaitForShutdown and block waiting for signals. But applications like
 * bitcoin-qt that need to run an event loop or otherwise not be idle can call
 * this to the receive the POSIX shutdown signal asynchronously.
 */
void HandleAsyncShutdown();

#endif
