---
title: Phase 11.5 Probe-3 — DFSPH upstream-source excerpts for commits 2 (a_ij coupling) and 3 (convergence)
date: 2026-05-14
audience: architect-1
role: probe-3 (read-only)
upstream: references/SPlisHSPlasH/ (SPlisHSPlasH 2.16.1)
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
scope: scalar DFSPH reference functions; AVX twins (TimeStepDFSPH.cpp:735-1103) intentionally skipped per architect-1 instruction
status: read-only — no edits, no builds, no binary runs
---

# Phase 11.5 Probe-3: DFSPH upstream reference excerpts

This probe quotes verbatim the SPlisHSPlasH 2.16.1 reference implementation of DFSPH so architect-1 can write the equation-level GPU↔upstream mapping for commits 2 (a_ij coupling) and 3 (convergence check). All quotations are from `references/SPlisHSPlasH/SPlisHSPlasH/...`.

---

## Section A — Per-frame driver: `TimeStepDFSPH::step()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:117-249`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:117
void TimeStepDFSPH::step()
{
	Simulation *sim = Simulation::getCurrent();
	TimeManager *tm = TimeManager::getCurrent ();
	const Real h = tm->getTimeStepSize();
	const unsigned int nModels = sim->numberOfFluidModels();

	//////////////////////////////////////////////////////////////////////////
	// search the neighbors for all particles
	//////////////////////////////////////////////////////////////////////////
	sim->performNeighborhoodSearch();

#ifdef USE_PERFORMANCE_OPTIMIZATION
	//////////////////////////////////////////////////////////////////////////
	// precompute the values V_j * grad W_ij for all neighbors
	//////////////////////////////////////////////////////////////////////////
	START_TIMING("precomputeValues")
	precomputeValues();
	STOP_TIMING_AVG
#endif

	//////////////////////////////////////////////////////////////////////////
	// compute volume/density maps boundary contribution
	//////////////////////////////////////////////////////////////////////////
	if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
		computeVolumeAndBoundaryX();
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
		computeDensityAndGradient();

	//////////////////////////////////////////////////////////////////////////
	// compute densities
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nModels; fluidModelIndex++)
		computeDensities(fluidModelIndex);

	//////////////////////////////////////////////////////////////////////////
	// Compute the factor alpha_i for all particles i
	// using the equation (11) in [BK17]
	//////////////////////////////////////////////////////////////////////////
	START_TIMING("computeDFSPHFactor");
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nModels; fluidModelIndex++)
		computeDFSPHFactor(fluidModelIndex);
	STOP_TIMING_AVG;

	//////////////////////////////////////////////////////////////////////////
	// Perform divergence solve (see Algorithm 2 in [BK17])
	//////////////////////////////////////////////////////////////////////////
	if (m_enableDivergenceSolver)
	{
		START_TIMING("divergenceSolve");
		divergenceSolve();
		STOP_TIMING_AVG
	}
	else
		m_iterationsV = 0;

	//////////////////////////////////////////////////////////////////////////
	// Reset accelerations and add gravity
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nModels; fluidModelIndex++)
		clearAccelerations(fluidModelIndex);

	//////////////////////////////////////////////////////////////////////////
	// Compute all nonpressure forces like viscosity, vorticity, ...
	//////////////////////////////////////////////////////////////////////////
	sim->computeNonPressureForces();

	//////////////////////////////////////////////////////////////////////////
	// Update the time step size, e.g. by using a CFL condition
	//////////////////////////////////////////////////////////////////////////
	sim->updateTimeStepSize();

	//////////////////////////////////////////////////////////////////////////
	// compute new velocities only considering non-pressure forces
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int m = 0; m < nModels; m++)
	{
		FluidModel *fm = sim->getFluidModel(m);
		const unsigned int numParticles = fm->numActiveParticles();
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < (int)numParticles; i++)
			{
				if (fm->getParticleState(i) == ParticleState::Active)
				{
					Vector3r &vel = fm->getVelocity(i);
					vel += h * fm->getAcceleration(i);
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Perform constant density solve (see Algorithm 3 in [BK17])
	//////////////////////////////////////////////////////////////////////////
	START_TIMING("pressureSolve");
	pressureSolve();
	STOP_TIMING_AVG;

	//////////////////////////////////////////////////////////////////////////
	// compute final positions
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int m = 0; m < nModels; m++)
	{
		FluidModel *fm = sim->getFluidModel(m);
		const unsigned int numParticles = fm->numActiveParticles();
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < (int)numParticles; i++)
			{
				if (fm->getParticleState(i) == ParticleState::Active)
				{
					Vector3r &xi = fm->getPosition(i);
					const Vector3r &vi = fm->getVelocity(i);
					xi += h * vi;
				}
			}
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// emit new particles and perform an animation field step
	//////////////////////////////////////////////////////////////////////////
	sim->emitParticles();
	sim->animateParticles();

	//////////////////////////////////////////////////////////////////////////
	// Compute new time
	//////////////////////////////////////////////////////////////////////////
	tm->setTime (tm->getTime () + h);
}
```

---

## Section B — Outer loop: `TimeStepDFSPH::pressureSolve()` (density-constancy)

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:252-384`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:252
void TimeStepDFSPH::pressureSolve()
{
	const Real h = TimeManager::getCurrent()->getTimeStepSize();
	const Real h2 = h*h;
	const Real invH = static_cast<Real>(1.0) / h;
	const Real invH2 = static_cast<Real>(1.0) / h2;
	Simulation *sim = Simulation::getCurrent();
	const unsigned int nFluids = sim->numberOfFluidModels();

	//////////////////////////////////////////////////////////////////////////
	// Compute rho_adv
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
	{
		FluidModel *model = sim->getFluidModel(fluidModelIndex);
		const Real density0 = model->getDensity0();
		const int numParticles = (int)model->numActiveParticles();
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < numParticles; i++)
			{
				//////////////////////////////////////////////////////////////////////////
				// Compute rho_adv,i^(0) (see equation in Section 3.3 in [BK17])
				// using the velocities after the non-pressure forces were applied.
				//////////////////////////////////////////////////////////////////////////
				computeDensityAdv(fluidModelIndex, i, h, density0);

				//////////////////////////////////////////////////////////////////////////
				// In the end of Section 3.3 [BK17] we have to multiply the density 
				// error with the factor alpha_i divided by h^2. Hence, we multiply 
				// the factor directly by 1/h^2 here.
				//////////////////////////////////////////////////////////////////////////
				m_simulationData.getFactor(fluidModelIndex, i) *= invH2;

				//////////////////////////////////////////////////////////////////////////
				// For the warm start we use 0.5 times the old pressure value.
				// Note: We divide the value by h^2 since we multiplied it by h^2 at the end of 
				// the last time step to make it independent of the time step size.
				//////////////////////////////////////////////////////////////////////////
#ifdef USE_WARMSTART
				if (m_simulationData.getDensityAdv(fluidModelIndex, i) > 1.0)
					m_simulationData.getPressureRho2(fluidModelIndex, i) = static_cast<Real>(0.5) * min(m_simulationData.getPressureRho2(fluidModelIndex, i), static_cast<Real>(0.00025)) * invH2;
				else 
					m_simulationData.getPressureRho2(fluidModelIndex, i) = 0.0;
#else 
				//////////////////////////////////////////////////////////////////////////
				// If we don't use a warm start, we directly compute a pressure value
				// by multiplying the density error with the factor.
				//////////////////////////////////////////////////////////////////////////
				//m_simulationData.getPressureRho2(fluidModelIndex, i) = 0.0;
				const Real s_i = static_cast<Real>(1.0) - m_simulationData.getDensityAdv(fluidModelIndex, i);
				const Real residuum = min(s_i, static_cast<Real>(0.0));     // r = b - A*p
				m_simulationData.getPressureRho2(fluidModelIndex, i) = -residuum * m_simulationData.getFactor(fluidModelIndex, i);
#endif
			}
		}
	}

	m_iterations = 0;

	//////////////////////////////////////////////////////////////////////////
	// Start solver
	//////////////////////////////////////////////////////////////////////////
	
	Real avg_density_err = 0.0;
	bool chk = false;


	//////////////////////////////////////////////////////////////////////////
	// Perform solver iterations
	//////////////////////////////////////////////////////////////////////////
	while ((!chk || (m_iterations < m_minIterations)) && (m_iterations < m_maxIterations))
	{
		chk = true;
		for (unsigned int i = 0; i < nFluids; i++)
		{
			FluidModel *model = sim->getFluidModel(i);
			const Real density0 = model->getDensity0();

			avg_density_err = 0.0;
			pressureSolveIteration(i, avg_density_err);

			// Maximal allowed density fluctuation
			const Real eta = m_maxError * static_cast<Real>(0.01) * density0;  // maxError is given in percent
			chk = chk && (avg_density_err <= eta);
		}

		m_iterations++;
	}

	INCREASE_COUNTER("DFSPH - iterations", static_cast<Real>(m_iterations));

	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
	{
		FluidModel *model = sim->getFluidModel(fluidModelIndex);
		const int numParticles = (int)model->numActiveParticles();
		const Real density0 = model->getDensity0();
		
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < numParticles; i++)
			{
				//////////////////////////////////////////////////////////////////////////
				// Time integration of the pressure accelerations to get new velocities
				//////////////////////////////////////////////////////////////////////////
				computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2Data(), true);
				model->getVelocity(i) += h * m_simulationData.getPressureAccel(fluidModelIndex, i);
			}
		}
	}
#ifdef USE_WARMSTART
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
	{
		FluidModel* model = sim->getFluidModel(fluidModelIndex);
		const int numParticles = (int)model->numActiveParticles();
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < numParticles; i++)
			{
				//////////////////////////////////////////////////////////////////////////
				// Multiply by h^2, the time step size has to be removed 
				// to make the pressure value independent 
				// of the time step size
				//////////////////////////////////////////////////////////////////////////
				m_simulationData.getPressureRho2(fluidModelIndex, i) *= h2;
			}		
		}
	}
#endif
}
```

### B.observation — convergence-check identifiers

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The pressure-solve outer loop at `TimeStepDFSPH.cpp:324-341` references the following member variables and constants. Declaration sites listed:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_iterations` — `TimeStepDFSPH.h:29` (`unsigned int`). Initialized to 0 at `TimeStepDFSPH.cpp:34`. Reset at `TimeStepDFSPH.cpp:311`.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_minIterations` — `TimeStepDFSPH.h:31` (`unsigned int`). Initialized to `2` at `TimeStepDFSPH.cpp:35`. Parameter `MIN_ITERATIONS` registered at `TimeStepDFSPH.cpp:82-85` with min value 0.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_maxIterations` — `TimeStepDFSPH.h:32` (`unsigned int`). Initialized to `100` at `TimeStepDFSPH.cpp:36`. Parameter `MAX_ITERATIONS` at `TimeStepDFSPH.cpp:87-90` with min value 1.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_maxError` — `TimeStepDFSPH.h:30` (`Real`). Initialized to `0.01` at `TimeStepDFSPH.cpp:37`. Parameter `MAX_ERROR` at `TimeStepDFSPH.cpp:92-95` with min value `1e-6`. Comment at `TimeStepDFSPH.cpp:336` notes "maxError is given in percent".
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `avg_density_err` — local `Real`, `TimeStepDFSPH.cpp:317`. Recomputed each fluid model per iteration at `:332`; reduced inside `pressureSolveIteration` (see Section D).
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `chk` — local `bool`, `TimeStepDFSPH.cpp:318`. Reset to `true` at start of each iteration `:326`, ANDed with `(avg_density_err <= eta)` at `:337`.
- `eta` — local `Real`, computed per fluid as `m_maxError * 0.01 * density0` at `:336`.
- `density0` — per-fluid reference density from `model->getDensity0()` at `:330`.
- `m_simulationData.getFactor(...)` — α-factor scratch field (declared in `SimulationDataDFSPH.h`; multiplied by `invH2` at `:285` before the iteration loop).
- `m_simulationData.getDensityAdv(...)` — ρ_adv scratch.
- `m_simulationData.getPressureRho2(...)` — p/ρ² scratch.

Loop guard (literal text): `while ((!chk || (m_iterations < m_minIterations)) && (m_iterations < m_maxIterations))`.

---

## Section C — Outer loop: `TimeStepDFSPH::divergenceSolve()` (divergence-free)

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:386-541`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:386
void TimeStepDFSPH::divergenceSolve()
{
	//////////////////////////////////////////////////////////////////////////
	// Init parameters
	//////////////////////////////////////////////////////////////////////////

	const Real h = TimeManager::getCurrent()->getTimeStepSize();
	const Real invH = static_cast<Real>(1.0) / h;
	Simulation *sim = Simulation::getCurrent();
	const unsigned int maxIter = m_maxIterationsV;
	const Real maxError = m_maxErrorV;
	const unsigned int nFluids = sim->numberOfFluidModels();

	//////////////////////////////////////////////////////////////////////////
	// Compute divergence of velocity field
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
	{
		FluidModel *model = sim->getFluidModel(fluidModelIndex);
		const int numParticles = (int)model->numActiveParticles();

		#pragma omp parallel default(shared)
		{		
			#pragma omp for schedule(static)  
			for (int i = 0; i < numParticles; i++)
			{
				//////////////////////////////////////////////////////////////////////////
				// Compute rho_adv,i^(0) (see equation (9) in Section 3.2 [BK17])
				// using the velocities after the non-pressure forces were applied.
				//////////////////////////////////////////////////////////////////////////
				computeDensityChange(fluidModelIndex, i, h);

				Real densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
				densityAdv = max(densityAdv, static_cast<Real>(0.0));

				unsigned int numNeighbors = 0;
				for (unsigned int pid = 0; pid < sim->numberOfPointSets(); pid++)
					numNeighbors += sim->numberOfNeighbors(fluidModelIndex, pid, i);

				// in case of particle deficiency do not perform a divergence solve
				if (!sim->is2DSimulation())
				{
					if (numNeighbors < 20)
						densityAdv = 0.0;
				}
				else
				{
					if (numNeighbors < 7)
						densityAdv = 0.0;
				}
				
				//////////////////////////////////////////////////////////////////////////
				// In equation (11) [BK17] we have to multiply the divergence 
				// error with the factor divided by h. Hence, we multiply the factor
				// directly by 1/h here.
				//////////////////////////////////////////////////////////////////////////
				m_simulationData.getFactor(fluidModelIndex, i) *= invH;

				//////////////////////////////////////////////////////////////////////////
				// For the warm start we use 0.5 times the old pressure value.
				// Divide the value by h. We multiplied it by h at the end of 
				// the last time step to make it independent of the time step size.
				//////////////////////////////////////////////////////////////////////////
#ifdef USE_WARMSTART_V
				if (densityAdv > 0.0)
					m_simulationData.getPressureRho2_V(fluidModelIndex, i) = static_cast<Real>(0.5) * min(m_simulationData.getPressureRho2_V(fluidModelIndex, i), static_cast<Real>(0.5)) * invH;
				else
					m_simulationData.getPressureRho2_V(fluidModelIndex, i) = 0.0;
#else 
				//////////////////////////////////////////////////////////////////////////
				// If we don't use a warm start, directly compute a pressure value
				// by multiplying the divergence error with the factor.
				//////////////////////////////////////////////////////////////////////////
				m_simulationData.getPressureRho2_V(fluidModelIndex, i) = densityAdv * m_simulationData.getFactor(fluidModelIndex, i);
#endif
			}
		}
	}

	m_iterationsV = 0;

	//////////////////////////////////////////////////////////////////////////
	// Start solver
	//////////////////////////////////////////////////////////////////////////
	
	Real avg_density_err = 0.0;
	bool chk = false;

	//////////////////////////////////////////////////////////////////////////
	// Perform solver iterations
	//////////////////////////////////////////////////////////////////////////
	while ((!chk || (m_iterationsV < 1)) && (m_iterationsV < maxIter))
	{
		chk = true;
		for (unsigned int i = 0; i < nFluids; i++)
		{
			FluidModel *model = sim->getFluidModel(i);
			const Real density0 = model->getDensity0();

			avg_density_err = 0.0;
			divergenceSolveIteration(i, avg_density_err);

			// Maximal allowed density fluctuation
			// use maximal density error divided by time step size
			const Real eta = (static_cast<Real>(1.0) / h) * maxError * static_cast<Real>(0.01) * density0;  // maxError is given in percent
			chk = chk && (avg_density_err <= eta);
		}

		m_iterationsV++;
	}

	INCREASE_COUNTER("DFSPH - iterationsV", static_cast<Real>(m_iterationsV));

	
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
	{
		FluidModel *model = sim->getFluidModel(fluidModelIndex);
		const int numParticles = (int)model->numActiveParticles();
		const Real density0 = model->getDensity0();

		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < numParticles; i++)
			{
				//////////////////////////////////////////////////////////////////////////
				// Time integration of the pressure accelerations
				//////////////////////////////////////////////////////////////////////////
				computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2VData(), true);
				model->getVelocity(i) += h * m_simulationData.getPressureAccel(fluidModelIndex, i);

				m_simulationData.getFactor(fluidModelIndex, i) *= h;
			}
		}
	}
#ifdef USE_WARMSTART_V
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nFluids; fluidModelIndex++)
	{
		FluidModel* model = sim->getFluidModel(fluidModelIndex);
		const int numParticles = (int)model->numActiveParticles();
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < numParticles; i++)
			{
				//////////////////////////////////////////////////////////////////////////
				// Multiply by h, the time step size has to be removed 
				// to make the pressure value independent 
				// of the time step size
				//////////////////////////////////////////////////////////////////////////		
				m_simulationData.getPressureRho2_V(fluidModelIndex, i) *= h;
			}
		}
	}
#endif
}
```

### C.observation — divergence-solve convergence identifiers

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The divergence outer loop at `TimeStepDFSPH.cpp:477-495` references:

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_iterationsV` — `TimeStepDFSPH.h:34` (`unsigned int`). Init 0 at `TimeStepDFSPH.cpp:38`. Reset at `:465`.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_maxIterationsV` — `TimeStepDFSPH.h:36` (`unsigned int`). Init 100 at `TimeStepDFSPH.cpp:40`. Aliased to local `maxIter` at `:395`. Parameter `MAX_ITERATIONS_V` registered at `TimeStepDFSPH.cpp:102-105`.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_maxErrorV` — `TimeStepDFSPH.h:35` (`Real`). Init 0.1 at `TimeStepDFSPH.cpp:41`. Aliased to local `maxError` at `:396`. Parameter `MAX_ERROR_V` at `:107-110`.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- `m_enableDivergenceSolver` — `TimeStepDFSPH.h:33` (`bool`). Init true at `TimeStepDFSPH.cpp:39`. Read at `:164` in `step()` to gate the divergence solve.
- `eta` — local `Real`, formula at `:490`: `(1/h) * maxError * 0.01 * density0`. Note: contrast with pressure-solve's `eta = m_maxError * 0.01 * density0` (no 1/h factor) at `:336`.
- Loop guard: `while ((!chk || (m_iterationsV < 1)) && (m_iterationsV < maxIter))`. The minimum-iterations constraint here is the hard-coded literal `1`, **not** a member variable. (Compare pressure-solve which uses `m_minIterations` (= 2 by default).)
- Particle-deficiency guard at `:426-435`: if total neighbors across all point sets is < 20 (3D) or < 7 (2D), set `densityAdv = 0` so the particle does not contribute. Replicated inside `divergenceSolveIteration` at `:676-690`.

---

## Section D — Inner iteration: `TimeStepDFSPH::pressureSolveIteration()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:544-619`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:544
void TimeStepDFSPH::pressureSolveIteration(const unsigned int fluidModelIndex, Real &avg_density_err)
{
	Simulation *sim = Simulation::getCurrent();
	FluidModel *model = sim->getFluidModel(fluidModelIndex);
	const Real density0 = model->getDensity0();
	const int numParticles = (int)model->numActiveParticles();
	if (numParticles == 0)
		return;

	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();
	const Real h = TimeManager::getCurrent()->getTimeStepSize();
	const Real invH = static_cast<Real>(1.0) / h;
	
	Real density_error = 0.0;

	#pragma omp parallel default(shared)
	{
		//////////////////////////////////////////////////////////////////////////
		// Compute pressure accelerations using the current pressure values.
		// (see Algorithm 3, line 7 in [BK17])
		//////////////////////////////////////////////////////////////////////////
		#pragma omp for schedule(static) 
		for (int i = 0; i < numParticles; i++)
		{
			computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2Data());
		}

		//////////////////////////////////////////////////////////////////////////
		// Update pressure values
		//////////////////////////////////////////////////////////////////////////
		#pragma omp for reduction(+:density_error) schedule(static) 
		for (int i = 0; i < numParticles; i++)
		{						
			if (model->getParticleState(i) != ParticleState::Active)
				continue;
				
			Real aij_pj = compute_aij_pj(fluidModelIndex, i);
			aij_pj *= h * h;

			//////////////////////////////////////////////////////////////////////////
			// Compute source term: s_i = 1 - rho_adv
			// Note: that due to our multiphase handling, the multiplier rho0
			// is missing here
			//////////////////////////////////////////////////////////////////////////
			const Real& densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
			const Real s_i = static_cast<Real>(1.0) - densityAdv;


			//////////////////////////////////////////////////////////////////////////
			// Update the value p/rho^2 (in [BK17] this is kappa/rho):
			// 
			// alpha_i = -1 / (a_ii * rho_i^2)
			// p_rho2_i = (p_i / rho_i^2)
			// 
			// Therefore, the following lines compute the Jacobi iteration:
			// p_i := p_i + 1/a_ii (source_term_i - a_ij * p_j)
			//////////////////////////////////////////////////////////////////////////
			Real& p_rho2_i = m_simulationData.getPressureRho2(fluidModelIndex, i);
			const Real residuum = min(s_i - aij_pj, static_cast<Real>(0.0));     // r = b - A*p
			//p_rho2_i -= residuum * m_simulationData.getFactor(fluidModelIndex, i);

			p_rho2_i = max(p_rho2_i - static_cast<Real>(0.5) * (s_i - aij_pj) * m_simulationData.getFactor(fluidModelIndex, i), static_cast<Real>(0.0));

			//////////////////////////////////////////////////////////////////////////
			// Compute the sum of the density errors
			//////////////////////////////////////////////////////////////////////////
			density_error -= density0 * residuum;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Compute the average density error
	//////////////////////////////////////////////////////////////////////////
	avg_density_err = density_error / numParticles;
}
```

### D.calls

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- Call: `computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2Data())` @ line 569 (no `applyBoundaryForces` arg → defaults to `false` per `TimeStepDFSPH.h:46`).
- Call: `compute_aij_pj(fluidModelIndex, i)` @ line 581.
- Member access (read): `m_simulationData.getDensityAdv(fluidModelIndex, i)` @ line 589.
- Member access (read+write): `m_simulationData.getPressureRho2(fluidModelIndex, i)` @ line 602 (returns `Real&`), assigned at line 606.
- Member access (read): `m_simulationData.getFactor(fluidModelIndex, i)` @ line 606.
- Particle-state guard: `model->getParticleState(i) != ParticleState::Active` @ line 578.
- Reduction: `#pragma omp for reduction(+:density_error)` @ line 575; final divide by `numParticles` @ line 618.

---

## Section E — Inner iteration: `TimeStepDFSPH::divergenceSolveIteration()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:621-706`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:621
void TimeStepDFSPH::divergenceSolveIteration(const unsigned int fluidModelIndex, Real &avg_density_err)
{
	Simulation *sim = Simulation::getCurrent();
	FluidModel *model = sim->getFluidModel(fluidModelIndex);
	const Real density0 = model->getDensity0();
	const int numParticles = (int)model->numActiveParticles();
	if (numParticles == 0)
		return;

	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();
	const Real h = TimeManager::getCurrent()->getTimeStepSize();
	const Real invH = static_cast<Real>(1.0) / h;
	
	Real density_error = 0.0;
	
	#pragma omp parallel default(shared)
	{
		//////////////////////////////////////////////////////////////////////////
		// Compute pressure accelerations using the current pressure values.
 		// (see Algorithm 2, line 7 in [BK17])
		//////////////////////////////////////////////////////////////////////////
		#pragma omp for schedule(static) 
		for (int i = 0; i < (int)numParticles; i++)
		{
			computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2VData());
		}

		//////////////////////////////////////////////////////////////////////////
		// Update pressure 
		//////////////////////////////////////////////////////////////////////////
		#pragma omp for reduction(+:density_error) schedule(static) 
		for (int i = 0; i < numParticles; i++)
		{
			Real aij_pj = compute_aij_pj(fluidModelIndex, i);
			aij_pj *= h;

			//////////////////////////////////////////////////////////////////////////
			// Compute source term: s_i = -d rho / dt
			//////////////////////////////////////////////////////////////////////////
			const Real& densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
			const Real s_i = -densityAdv;

			//////////////////////////////////////////////////////////////////////////
			// Update the value p/rho^2:
			// 
			// alpha_i = -1 / (a_ii * rho_i^2)
			// pv_rho2_i = (pv_i / rho_i^2)
			// 
			// Therefore, the following line computes the Jacobi iteration:
			// pv_i := pv_i + 1/a_ii (source_term_i - a_ij * pv_j)
			//////////////////////////////////////////////////////////////////////////
			Real& pv_rho2_i = m_simulationData.getPressureRho2_V(fluidModelIndex, i);
			Real residuum = min(s_i - aij_pj, static_cast<Real>(0.0));     // r = b - A*p

			unsigned int numNeighbors = 0;
			for (unsigned int pid = 0; pid < sim->numberOfPointSets(); pid++)
				numNeighbors += sim->numberOfNeighbors(fluidModelIndex, pid, i);

			// in case of particle deficiency do not perform a divergence solve
			if (!sim->is2DSimulation())
			{
				if (numNeighbors < 20)
					residuum = 0.0;
			}
			else
			{
				if (numNeighbors < 7)
					residuum = 0.0;
			}
			//pv_rho2_i -= residuum * m_simulationData.getFactor(fluidModelIndex, i);
			pv_rho2_i = max(pv_rho2_i - static_cast<Real>(0.5)*(s_i - aij_pj) * m_simulationData.getFactor(fluidModelIndex, i), static_cast<Real>(0.0));


			//////////////////////////////////////////////////////////////////////////
			// Compute the sum of the divergence errors
			//////////////////////////////////////////////////////////////////////////
			density_error -= density0 * residuum;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	// Compute the average divergence error
	//////////////////////////////////////////////////////////////////////////
	avg_density_err = density_error / numParticles;
}
```

### E.calls

- Call: `computePressureAccel(fluidModelIndex, i, density0, m_simulationData.getPressureRho2VData())` @ line 646 (defaults `applyBoundaryForces = false`). Note `getPressureRho2VData()` (divergence pressure) vs `getPressureRho2Data()` in D.
- Call: `compute_aij_pj(fluidModelIndex, i)` @ line 655. Multiplied by `h` (not `h*h`) at line 656 — contrast with `aij_pj *= h*h` at `:582` in the density solver.
- Member access (read): `m_simulationData.getDensityAdv(fluidModelIndex, i)` @ line 661. Here `s_i = -densityAdv` (no `1.0 -` offset).
- Member access (read+write): `m_simulationData.getPressureRho2_V(fluidModelIndex, i)` @ line 673, written line 692.
- Member access (read): `m_simulationData.getFactor(fluidModelIndex, i)` @ line 692.
- Iter: `sim->numberOfNeighbors(...)` summed across all point sets @ lines 676-678; deficiency cutoffs at lines 681-690.
- Reduction: `#pragma omp for reduction(+:density_error)` @ line 652; final divide @ line 705.

Note: this function does **not** guard on `model->getParticleState(i) == Active`, unlike `pressureSolveIteration` at `:578`.

---

## Section F — Scalar `computeDFSPHFactor()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1106-1186`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1106
void TimeStepDFSPH::computeDFSPHFactor(const unsigned int fluidModelIndex)
{
	//////////////////////////////////////////////////////////////////////////
	// Init parameters
	//////////////////////////////////////////////////////////////////////////

	Simulation *sim = Simulation::getCurrent();
	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();
	FluidModel *model = sim->getFluidModel(fluidModelIndex);
	const int numParticles = (int) model->numActiveParticles();

	#pragma omp parallel default(shared)
	{
		//////////////////////////////////////////////////////////////////////////
		// Compute pressure stiffness denominator
		//////////////////////////////////////////////////////////////////////////

		#pragma omp for schedule(static)  
		for (int i = 0; i < numParticles; i++)
		{
			//////////////////////////////////////////////////////////////////////////
			// Compute gradient dp_i/dx_j * (1/kappa)  and dp_j/dx_j * (1/kappa)
			// (see Equation (8) and the previous one [BK17])
			// Note: That in all quantities rho0 is missing due to our
			// implementation of multiphase simulations.
			//////////////////////////////////////////////////////////////////////////
			const Vector3r &xi = model->getPosition(i);
			Real sum_grad_p_k = 0.0;
			Vector3r grad_p_i;
			grad_p_i.setZero();

			//////////////////////////////////////////////////////////////////////////
			// Fluid
			//////////////////////////////////////////////////////////////////////////
			forall_fluid_neighbors(
				const Vector3r grad_p_j = -fm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj);
				sum_grad_p_k += grad_p_j.squaredNorm();
				grad_p_i -= grad_p_j;
			);
			
			//////////////////////////////////////////////////////////////////////////
			// Boundary
			//////////////////////////////////////////////////////////////////////////
			if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
			{
				forall_boundary_neighbors(
					const Vector3r grad_p_j = -bm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj);
					grad_p_i -= grad_p_j;
				);
			}

			else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
			{
				forall_density_maps(
					grad_p_i -= gradRho;
				);
			}
			else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
			{
				forall_volume_maps(
					const Vector3r grad_p_j = -Vj * sim->gradW(xi - xj);
					grad_p_i -= grad_p_j;
				);
			}		

			sum_grad_p_k += grad_p_i.squaredNorm();

			//////////////////////////////////////////////////////////////////////////
			// Compute factor as: factor_i = -1 / (a_ii * rho_i^2)
			// where a_ii is the diagonal entry of the linear system 
			// for the pressure A * p = source term
			//////////////////////////////////////////////////////////////////////////
			Real &factor = m_simulationData.getFactor(fluidModelIndex, i);
			if (sum_grad_p_k > m_eps)
				factor = static_cast<Real>(1.0) / (sum_grad_p_k);
			else
				factor = 0.0;
		}
	}
}
```

### F.observations

- Macros used:
  - `forall_fluid_neighbors` @ line 1141
  - `forall_boundary_neighbors` @ line 1152 (Akinci2012 branch)
  - `forall_density_maps` @ line 1160 (Koschier2017 branch)
  - `forall_volume_maps` @ line 1166 (Bender2019 branch)
- Boundary branches present: all three (`Akinci2012`, `Koschier2017`, `Bender2019`).
- Contributions to `sum_grad_p_k` vs `grad_p_i`:
  - **Fluid neighbors** contribute to BOTH `sum_grad_p_k` (via `grad_p_j.squaredNorm()`) AND `grad_p_i` (via `-= grad_p_j`).
  - **All three boundary branches** contribute ONLY to `grad_p_i` (via `-= grad_p_j` or `-= gradRho`). The per-neighbor `squaredNorm` is NOT accumulated into `sum_grad_p_k` for boundary neighbors.
  - Then at line 1172, `sum_grad_p_k += grad_p_i.squaredNorm()` adds the squared norm of the accumulated `grad_p_i` (which has had all fluid + boundary contributions subtracted).
- Final factor: `factor_i = 1 / sum_grad_p_k` if above `m_eps`, else 0. Comment says `factor_i = -1 / (a_ii * rho_i^2)`; sign is absorbed elsewhere by the solver iteration logic.

---

## Section G — Scalar `computeDensityAdv()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1188-1242`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1188
/** Compute rho_adv,i^(0) (see equation in Section 3.3 in [BK17])
  * using the velocities after the non-pressure forces were applied.
**/
void TimeStepDFSPH::computeDensityAdv(const unsigned int fluidModelIndex, const unsigned int i, const Real h, const Real density0)
{
	Simulation *sim = Simulation::getCurrent();
	FluidModel *model = sim->getFluidModel(fluidModelIndex);
	const Real &density = model->getDensity(i);
	Real &densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
	const Vector3r &xi = model->getPosition(i);
	const Vector3r &vi = model->getVelocity(i);
	Real delta = 0.0;
	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();

	//////////////////////////////////////////////////////////////////////////
	// Fluid
	//////////////////////////////////////////////////////////////////////////
	forall_fluid_neighbors(
		const Vector3r & vj = fm_neighbor->getVelocity(neighborIndex);
		delta += (vi - vj).dot(sim->gradW(xi - xj));
		//delta += fm_neighbor->getVolume(neighborIndex) * (vi - vj).dot(sim->gradW(xi - xj));
	);
	// assumes that all fluid particles have the same volume
	delta *= model->getVolume(i);

	//////////////////////////////////////////////////////////////////////////
	// Boundary
	//////////////////////////////////////////////////////////////////////////
	if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
	{
		forall_boundary_neighbors(
			const Vector3r &vj = bm_neighbor->getVelocity(neighborIndex);
			delta += bm_neighbor->getVolume(neighborIndex) * (vi - vj).dot(sim->gradW(xi - xj));
		);
	}
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
	{
		forall_density_maps(
			Vector3r vj;
			bm_neighbor->getPointVelocity(xi, vj);
			delta -= (vi - vj).dot(gradRho);
		);
	}
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
	{
		forall_volume_maps(
			Vector3r vj;
			bm_neighbor->getPointVelocity(xj, vj);
			delta += Vj * (vi - vj).dot(sim->gradW(xi - xj));
		);
	}

	densityAdv = density / density0 + h*delta;
}
```

### G.observations

- Fluid contribution: `delta += (vi - vj) · ∇W(xi - xj)`, then `delta *= model->getVolume(i)` (uniform-volume assumption — see line 1211 comment).
- Boundary branches: all three add to `delta`. Akinci2012 weights by `bm_neighbor->getVolume(j)`; Koschier2017 uses `-= ... · gradRho` (sign flipped, density-map gradient); Bender2019 uses `Vj` from the volume map.
- Final assignment: `densityAdv = density / density0 + h * delta` (line 1241).

---

## Section H — Scalar `computeDensityChange()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1244-1295`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1244
/** Compute rho_adv,i^(0) (see equation (9) in Section 3.2 [BK17])
  * using the velocities after the non-pressure forces were applied.
  */
void TimeStepDFSPH::computeDensityChange(const unsigned int fluidModelIndex, const unsigned int i, const Real h)
{
	Simulation *sim = Simulation::getCurrent();
	FluidModel *model = sim->getFluidModel(fluidModelIndex);
	Real &densityAdv = m_simulationData.getDensityAdv(fluidModelIndex, i);
	const Vector3r &xi = model->getPosition(i);
	const Vector3r& vi = model->getVelocity(i);
	densityAdv = 0.0;
	unsigned int numNeighbors = 0;
	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();

	//////////////////////////////////////////////////////////////////////////
	// Fluid
	//////////////////////////////////////////////////////////////////////////
	forall_fluid_neighbors(
		const Vector3r & vj = fm_neighbor->getVelocity(neighborIndex);
		densityAdv += (vi - vj).dot(sim->gradW(xi - xj));
	);
	// assumes that all fluid particles have the same volume
	densityAdv *= model->getVolume(i);

	//////////////////////////////////////////////////////////////////////////
	// Boundary
	//////////////////////////////////////////////////////////////////////////
	if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
	{
		forall_boundary_neighbors(
			const Vector3r &vj = bm_neighbor->getVelocity(neighborIndex);
			densityAdv += bm_neighbor->getVolume(neighborIndex) * (vi - vj).dot(sim->gradW(xi - xj));
		);
	}
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
	{
		forall_density_maps(
			Vector3r vj;
			bm_neighbor->getPointVelocity(xi, vj);
			densityAdv -= (vi - vj).dot(gradRho);
		);
	}
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
	{
		forall_volume_maps(
			Vector3r vj;
			bm_neighbor->getPointVelocity(xj, vj);
			densityAdv += Vj * (vi - vj).dot(sim->gradW(xi - xj));
		);
	}
}
```

### H.observations

- All three boundary branches (Akinci2012, Koschier2017, Bender2019) contribute. Forms identical to `computeDensityAdv` except writing into `densityAdv` directly instead of an intermediate `delta`.
- Local `numNeighbors` is declared at line 1255 but never written in the scalar variant (note: the corresponding `divergenceSolve()` outer-loop body at `:421-423` and `divergenceSolveIteration` at `:676-678` recompute neighbor counts; this function does NOT do its own deficiency clamp).
- No final `density/density0` term — pure divergence ρ̇.

---

## Section I — Scalar `computePressureAccel()`

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1297-1367`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1297
/** Compute pressure accelerations using the current pressure values of the particles
 */
void TimeStepDFSPH::computePressureAccel(const unsigned int fluidModelIndex, const unsigned int i, const Real density0, std::vector<std::vector<Real>>& pressure_rho2, const bool applyBoundaryForces)
{
	Simulation* sim = Simulation::getCurrent();
	FluidModel* model = sim->getFluidModel(fluidModelIndex);
	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();

	Vector3r& ai = m_simulationData.getPressureAccel(fluidModelIndex, i);
	ai.setZero();

	if (model->getParticleState(i) != ParticleState::Active)
		return;

	// p_rho2_i = (p_i / rho_i^2)
	const Real p_rho2_i = pressure_rho2[fluidModelIndex][i];
	const Vector3r &xi = model->getPosition(i);

	//////////////////////////////////////////////////////////////////////////
	// Fluid
	//////////////////////////////////////////////////////////////////////////
	forall_fluid_neighbors(			
		// p_rho2_j = (p_j / rho_j^2)
		const Real p_rho2_j = pressure_rho2[pid][neighborIndex];
		const Real pSum = p_rho2_i + fm_neighbor->getDensity0()/density0 * p_rho2_j;
		if (fabs(pSum) > m_eps)
		{
			const Vector3r grad_p_j = -fm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj);
			ai += pSum * grad_p_j;		
		}
	)

	//////////////////////////////////////////////////////////////////////////
	// Boundary
	//////////////////////////////////////////////////////////////////////////
	if (fabs(p_rho2_i) > m_eps)
	{
		if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
		{
			forall_boundary_neighbors(
				const Vector3r grad_p_j = -bm_neighbor->getVolume(neighborIndex) * sim->gradW(xi - xj);

				const Vector3r a = (Real) 1.0 * p_rho2_i * grad_p_j;		
				ai += a;
				if (applyBoundaryForces)
					bm_neighbor->addForce(xj, -model->getMass(i) * a);
			);
		}
		else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
		{
			forall_density_maps(
				const Vector3r a = (Real) 1.0 * p_rho2_i * gradRho;			
				ai += a;
				if (applyBoundaryForces)
					bm_neighbor->addForce(xj, -model->getMass(i) * a);
			);
		}
		else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
		{
			forall_volume_maps(
				const Vector3r grad_p_j = -Vj * sim->gradW(xi - xj);
				const Vector3r a = (Real) 1.0 * p_rho2_i * grad_p_j;		
				ai += a;

				if (applyBoundaryForces)
					bm_neighbor->addForce(xj, -model->getMass(i) * a);  
			);
		}
	}
}
```

### I.observations

- Boundary branches: all three present, each gated on `fabs(p_rho2_i) > m_eps`.
- Notable asymmetry: the fluid term uses `pSum = p_rho2_i + (ρ0_j/ρ0_i) * p_rho2_j` whereas the boundary terms use only `p_rho2_i` (treating boundary pressure as the symmetric partner of the fluid particle's own pressure). This is the standard Akinci-style symmetrization.
- The `applyBoundaryForces` flag (default `false` in header) triggers `bm_neighbor->addForce(xj, ...)` — i.e., Newton's-third-law reaction onto the rigid body. Called as `true` from `pressureSolve` (`:359`) and `divergenceSolve` (`:514`) but `false` inside the iteration (`:569`, `:646`).
- Particle-state guard at line 1309: inactive particles get zeroed acceleration and early return.

---

## Section J — Scalar `compute_aij_pj()` — central function for commit 2

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1370-1420`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:1370
Real TimeStepDFSPH::compute_aij_pj(const unsigned int fluidModelIndex, const unsigned int i)
{
	Simulation* sim = Simulation::getCurrent();
	FluidModel* model = sim->getFluidModel(fluidModelIndex);
	const unsigned int nFluids = sim->numberOfFluidModels();
	const unsigned int nBoundaries = sim->numberOfBoundaryModels();

	//////////////////////////////////////////////////////////////////////////
	// Compute A*p which is the change of the density when applying the 
	// pressure forces. 
	// \sum_j a_ij * p_j = h^2 \sum_j V_j (a_i - a_j) * gradW_ij
	// This is the RHS of Equation (12) in [BK17]
	//////////////////////////////////////////////////////////////////////////
	const Vector3r& xi = model->getPosition(i);
	const Vector3r& ai = m_simulationData.getPressureAccel(fluidModelIndex, i);
	Real aij_pj = 0.0;

	//////////////////////////////////////////////////////////////////////////
	// Fluid
	//////////////////////////////////////////////////////////////////////////
	forall_fluid_neighbors(
		const Vector3r & aj = m_simulationData.getPressureAccel(pid, neighborIndex);
		//aij_pj += fm_neighbor->getVolume(neighborIndex) * (ai - aj).dot(sim->gradW(xi - xj));
		aij_pj += (ai - aj).dot(sim->gradW(xi - xj));
	);
	// assumes that all fluid particles have the same volume
	aij_pj *= model->getVolume(i);

	//////////////////////////////////////////////////////////////////////////
	// Boundary
	//////////////////////////////////////////////////////////////////////////
	if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Akinci2012)
	{
		forall_boundary_neighbors(
			aij_pj += bm_neighbor->getVolume(neighborIndex) * ai.dot(sim->gradW(xi - xj));
		);
	}
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Koschier2017)
	{
		forall_density_maps(
			aij_pj -= ai.dot(gradRho);
		);
	}
	else if (sim->getBoundaryHandlingMethod() == BoundaryHandlingMethods::Bender2019)
	{
		forall_volume_maps(
			aij_pj += Vj * ai.dot(sim->gradW(xi - xj));
		);
	}
	return aij_pj;
}
```

### J.io-decomposition

**Inputs (reads):**
- Parameters: `fluidModelIndex` (unsigned int), `i` (unsigned int — particle index in this fluid model).
- `Simulation::getCurrent()` singleton; `nFluids`, `nBoundaries` from it.
- `model->getPosition(i)` → `xi`.
- `model->getVolume(i)` (uniform-volume scaling at line 1396).
- `m_simulationData.getPressureAccel(fluidModelIndex, i)` → `ai` (pressure acceleration of particle i, computed by the most recent `computePressureAccel` call inside the parallel iteration).
- Inside `forall_fluid_neighbors`: `m_simulationData.getPressureAccel(pid, neighborIndex)` → `aj`. Also implicit: `fm_neighbor` (from macro), `xj` (from macro).
- Inside `forall_boundary_neighbors`: `bm_neighbor->getVolume(neighborIndex)`, `xj` (from macro).
- Inside `forall_density_maps`: `gradRho` (from macro).
- Inside `forall_volume_maps`: `Vj` (from macro), `xj`.
- `sim->gradW(xi - xj)` — global gradient-W kernel dispatch.
- `sim->getBoundaryHandlingMethod()` for branch selection.

**Returns:** a single `Real` — the scalar quantity `Σ_j a_ij * p_j` (the RHS of [BK17] Eq. 12, pre-multiplication by `h²` or `h` — that scaling is applied by the caller at `:582` (×h²) and `:656` (×h)).

**Macros used:**
- `forall_fluid_neighbors` @ line 1390
- `forall_boundary_neighbors` @ line 1403 (Akinci2012)
- `forall_density_maps` @ line 1409 (Koschier2017)
- `forall_volume_maps` @ line 1415 (Bender2019)

**Boundary branches:** all three present.

### J.algebraic-structure

Walking the additive accumulation in order:

1. **Initialize** `aij_pj = 0` (line 1385).
2. **Fluid neighbor loop** (line 1390-1394): for each fluid neighbor j across all phases,
    ```
    aij_pj += (ai - aj) · ∇W(xi - xj)
    ```
    Note the commented-out alternative `aij_pj += fm_neighbor->getVolume(j) * (ai - aj) · ∇W` indicates the per-neighbor volume factor has been factored out and is applied uniformly below — assumes all fluid particles have identical volume.
3. **Uniform-volume scaling** (line 1396): `aij_pj *= model->getVolume(i)`. So after this step, `aij_pj = V_i * Σ_j (a_i - a_j) · ∇W_ij`.
4. **Boundary branch — exclusive selection by `getBoundaryHandlingMethod()`:**
    - If `Akinci2012` (line 1401-1406): for each boundary particle j,
      ```
      aij_pj += V_j(boundary) * ai · ∇W(xi - xj)
      ```
      (Uses only `ai`, not `(ai - aj)` — boundary particle is treated as having zero pressure acceleration aj.)
    - Else if `Koschier2017` (line 1407-1412): for each density-map boundary,
      ```
      aij_pj -= ai · gradRho
      ```
      (Subtraction; uses the density-map gradient.)
    - Else if `Bender2019` (line 1413-1418): for each volume-map boundary,
      ```
      aij_pj += Vj(volume-map) * ai · ∇W(xi - xj)
      ```
      (Volume from the volume map; otherwise identical form to Akinci2012.)
5. **Return** `aij_pj` (line 1419).

Caller-side scaling (`*= h*h` at `:582` density solver, `*= h` at `:656` divergence solver) finishes the [BK17] Eq. (12) RHS.

---

## Section K — Neighbor-loop macro definitions

All scalar (non-AVX) `forall_*` macros live in `Simulation.h`. Quoting verbatim:

### K.1 `forall_fluid_neighbors`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/Simulation.h:19-32
/** Loop over the fluid neighbors of all fluid phases. 
* Simulation *sim and unsigned int fluidModelIndex must be defined.
*/
#define forall_fluid_neighbors(code) \
	for (unsigned int pid = 0; pid < nFluids; pid++) \
	{ \
		FluidModel *fm_neighbor = sim->getFluidModelFromPointSet(pid); \
		for (unsigned int j = 0; j < sim->numberOfNeighbors(fluidModelIndex, pid, i); j++) \
		{ \
			const unsigned int neighborIndex = sim->getNeighbor(fluidModelIndex, pid, i, j); \
			const Vector3r &xj = fm_neighbor->getPosition(neighborIndex); \
			code \
		} \
	} 
```

### K.2 `forall_fluid_neighbors_in_same_phase`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/Simulation.h:34-43
/** Loop over the fluid neighbors of the same fluid phase.
* Simulation *sim, unsigned int fluidModelIndex and FluidModel* model must be defined.
*/
#define forall_fluid_neighbors_in_same_phase(code) \
	for (unsigned int j = 0; j < sim->numberOfNeighbors(fluidModelIndex, fluidModelIndex, i); j++) \
	{ \
		const unsigned int neighborIndex = sim->getNeighbor(fluidModelIndex, fluidModelIndex, i, j); \
		const Vector3r &xj = model->getPosition(neighborIndex); \
		code \
	} 
```

### K.3 `forall_boundary_neighbors` (Akinci2012)

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/Simulation.h:45-58
/** Loop over the boundary neighbors of all fluid phases.
* Simulation *sim and unsigned int fluidModelIndex must be defined.
*/
#define forall_boundary_neighbors(code) \
for (unsigned int pid = nFluids; pid < sim->numberOfPointSets(); pid++) \
{ \
	BoundaryModel_Akinci2012 *bm_neighbor = static_cast<BoundaryModel_Akinci2012*>(sim->getBoundaryModelFromPointSet(pid)); \
	for (unsigned int j = 0; j < sim->numberOfNeighbors(fluidModelIndex, pid, i); j++) \
	{ \
		const unsigned int neighborIndex = sim->getNeighbor(fluidModelIndex, pid, i, j); \
		const Vector3r &xj = bm_neighbor->getPosition(neighborIndex); \
		code \
	} \
}
```

### K.4 `forall_density_maps` (Koschier2017)

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/Simulation.h:60-74
/** Loop over the boundary density maps.
* Simulation *sim, unsigned int nBoundaries and unsigned int fluidModelIndex must be defined.
*/
#define forall_density_maps(code) \
for (unsigned int pid = 0; pid < nBoundaries; pid++) \
{ \
	BoundaryModel_Koschier2017 *bm_neighbor = static_cast<BoundaryModel_Koschier2017*>(sim->getBoundaryModel(pid)); \
	const Real rho = bm_neighbor->getBoundaryDensity(fluidModelIndex, i); \
	if (rho != 0.0) \
	{ \
		const Vector3r &gradRho = bm_neighbor->getBoundaryDensityGradient(fluidModelIndex, i).cast<Real>(); \
		const Vector3r &xj = bm_neighbor->getBoundaryXj(fluidModelIndex, i); \
		code \
	} \
}
```

### K.5 `forall_volume_maps` (Bender2019)

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/Simulation.h:76-89
/** Loop over the boundary volume maps.
* Simulation *sim, unsigned int nBoundaries and unsigned int fluidModelIndex must be defined.
*/
#define forall_volume_maps(code) \
for (unsigned int pid = 0; pid < nBoundaries; pid++) \
{ \
	BoundaryModel_Bender2019 *bm_neighbor = static_cast<BoundaryModel_Bender2019*>(sim->getBoundaryModel(pid)); \
	const Real Vj = bm_neighbor->getBoundaryVolume(fluidModelIndex, i);  \
	if (Vj > 0.0) \
	{ \
		const Vector3r &xj = bm_neighbor->getBoundaryXj(fluidModelIndex, i); \
		code \
	} \
}
```

### K.summary — branch type mapping

| Macro | Boundary model | Iteration shape |
|---|---|---|
| `forall_boundary_neighbors` | Akinci2012 (rigid-body sampled particles) | Outer loop over boundary **point sets** with `pid = nFluids..numberOfPointSets()`, inner loop over **per-particle neighbors** via NHS. Each iteration gives a unique boundary particle index `neighborIndex` and `xj`. |
| `forall_density_maps` | Koschier2017 (density maps on rigid body) | Outer loop over **boundary models** with `pid = 0..nBoundaries`. Single boundary contribution per body (no inner neighbor loop) — provides one `gradRho` and one effective `xj` per body. Gated on `rho != 0.0`. |
| `forall_volume_maps` | Bender2019 (volume maps on rigid body) | Same structure as `forall_density_maps`: one contribution per boundary body, gated on `Vj > 0.0`. Provides `Vj` and `xj`. |

Note Akinci is the "true" boundary-particle loop (inner per-particle); the other two are per-rigid-body summary contributions from precomputed maps.

---

## Section L — Cubic spline kernel (`SPHKernels.h`)

`references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:14-91`

The full `CubicKernel` class declaration, including `setRadius`, `W`, `gradW`, and `W_zero`:

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/SPHKernels.h:14
	/** \brief Cubic spline kernel.
	*/
	class CubicKernel
	{
	protected:
		static Real m_radius;
		static Real m_k;
		static Real m_l;
		static Real m_W_zero;
	public:
		static Real getRadius() { return m_radius; }
		static void setRadius(Real val)
		{
			m_radius = val;
			const Real pi = static_cast<Real>(M_PI);

			const Real h3 = m_radius*m_radius*m_radius;
			m_k = static_cast<Real>(8.0) / (pi*h3);
			m_l = static_cast<Real>(48.0) / (pi*h3);
			m_W_zero = W(Vector3r::Zero());
		}

	public:
		static Real W(const Real r)
		{
			Real res = 0.0;
			const Real q = r / m_radius;
			if (q <= 1.0)
			{
				if (q <= 0.5)
				{
					const Real q2 = q*q;
					const Real q3 = q2*q;
					res = m_k * (static_cast<Real>(6.0)*q3 - static_cast<Real>(6.0)*q2 + static_cast<Real>(1.0));
				}
				else
				{
					res = m_k * (static_cast<Real>(2.0)*pow(static_cast<Real>(1.0) - q, static_cast<Real>(3.0)));
				}
			}
			return res;
		}

		static Real W(const Vector3r &r)
		{
			return W(r.norm());
		}

		static Vector3r gradW(const Vector3r &r)
		{
			Vector3r res;
			const Real rl = r.norm();
			const Real q = rl / m_radius;
			if ((rl > 1.0e-9) && (q <= 1.0))
			{
				Vector3r gradq = r / rl;
				gradq /= m_radius;
				if (q <= 0.5)
				{
					res = m_l*q*((Real) 3.0*q - static_cast<Real>(2.0))*gradq;
				}
				else
				{
					const Real factor = static_cast<Real>(1.0) - q;
					res = m_l*(-factor*factor)*gradq;
				}
			}
			else
				res.setZero();

			return res;
		}

		static Real W_zero()
		{
			return m_W_zero;
		}
	};
```

### L.observations — kernel normalization constants

Per `setRadius` (line 25-34) for the 3D cubic spline:

- `m_k = 8 / (π * h³)` — the W normalization constant.
- `m_l = 48 / (π * h³)` — the ∇W normalization constant. **Note both use `h3`** (h cubed), not `h³` for W and `h⁴` for ∇W. Probe-1 Section F (per architect-1 brief) reportedly computes `48/(π h⁴)` on the host side for ∇W — this is a **probable mismatch** worth flagging. (See Section P.)

The form `gradW(r) = m_l * q * (3q - 2) * (r / |r|) / h` for q≤0.5 collapses to `(48/(π h³)) * (3q²/h - 2q/h) * r̂ = ...`; the `1/h` from `gradq /= m_radius` is what would absorb the difference. But the **scalar constant `m_l` itself is `48/(π h³)`**, not `48/(π h⁴)`. The radial scale enters through `gradq = r / (rl * m_radius)` (lines 69-70).

### L.context — class boundaries

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The class begins at `SPHKernels.h:16` (preceded by a doc comment on line 14) and the closing `};` is at line 91. Static members are declared inside the class but defined (with their reset values) elsewhere — likely `SPHKernels.cpp`. This probe did not read `SPHKernels.cpp` per scope guardrails.

---

## Section M — `TimeStepDFSPH.h` (class declaration)

`references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h:1-79` (full file):

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.h:1
#ifndef __TimeStepDFSPH_h__
#define __TimeStepDFSPH_h__

#include "SPlisHSPlasH/Common.h"
#include "SPlisHSPlasH/TimeStep.h"
#include "SimulationDataDFSPH.h"
#include "SPlisHSPlasH/SPHKernels.h"

#define USE_WARMSTART
#define USE_WARMSTART_V

namespace SPH
{
	class SimulationDataDFSPH;

	/** \brief This class implements the Divergence-free Smoothed Particle Hydrodynamics approach introduced
	* by Bender and Koschier [BK15,BK17,KBST19].
	*
	* References:
	* - [BK15] Jan Bender and Dan Koschier. Divergence-free smoothed particle hydrodynamics. In ACM SIGGRAPH / Eurographics Symposium on Computer Animation, SCA '15, 147-155. New York, NY, USA, 2015. ACM. URL: http://doi.acm.org/10.1145/2786784.2786796
	* - [BK17] Jan Bender and Dan Koschier. Divergence-free SPH for incompressible and viscous fluids. IEEE Transactions on Visualization and Computer Graphics, 23(3):1193-1206, 2017. URL: http://dx.doi.org/10.1109/TVCG.2016.2578335
	* - [KBST19] Dan Koschier, Jan Bender, Barbara Solenthaler, and Matthias Teschner. Smoothed particle hydrodynamics for physically-based simulation of fluids and solids. In Eurographics 2019 - Tutorials. Eurographics Association, 2019. URL: https://interactivecomputergraphics.github.io/SPH-Tutorial
	*/
	class TimeStepDFSPH : public TimeStep
	{
	protected:
		SimulationDataDFSPH m_simulationData;
		const Real m_eps = static_cast<Real>(1.0e-5);
		unsigned int m_iterations;
		Real m_maxError;
		unsigned int m_minIterations;
		unsigned int m_maxIterations;
		bool m_enableDivergenceSolver;
		unsigned int m_iterationsV;
		Real m_maxErrorV;
		unsigned int m_maxIterationsV;

		void computeDFSPHFactor(const unsigned int fluidModelIndex);
		void pressureSolve();
		void pressureSolveIteration(const unsigned int fluidModelIndex, Real &avg_density_err);
		void divergenceSolve();
		void divergenceSolveIteration(const unsigned int fluidModelIndex, Real &avg_density_err);
		void computeDensityAdv(const unsigned int fluidModelIndex, const unsigned int index, const Real h, const Real density0);
		void computeDensityChange(const unsigned int fluidModelIndex, const unsigned int index, const Real h);

		void computePressureAccel(const unsigned int fluidModelIndex, const unsigned int i, const Real density0, std::vector<std::vector<Real>>& pressure_rho2, const bool applyBoundaryForces = false);
		Real compute_aij_pj(const unsigned int fluidModelIndex, const unsigned int i);

		virtual void performNeighborhoodSearchSort();
		virtual void emittedParticles(FluidModel *model, const unsigned int startIndex);

		/** Init all generic parameters */
		virtual void initParameters();

	public:
		static std::string METHOD_NAME;
		static int SOLVER_ITERATIONS;
		static int MIN_ITERATIONS;
		static int MAX_ITERATIONS;
		static int MAX_ERROR;
		static int SOLVER_ITERATIONS_V;
		static int MAX_ITERATIONS_V;
		static int MAX_ERROR_V;
		static int USE_DIVERGENCE_SOLVER;

		TimeStepDFSPH();
		virtual ~TimeStepDFSPH(void);

		/** perform a simulation step */
		virtual void step();
		virtual void reset();

		virtual void resize();
		virtual std::string getMethodName() { return METHOD_NAME; }
		virtual int getNumIterations() { return m_iterations; }
	};
}

#endif
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
### M.constructor-initialized values (`TimeStepDFSPH.cpp:29-42`)

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:29
TimeStepDFSPH::TimeStepDFSPH() :
	TimeStep(),
	m_simulationData()
{
	m_simulationData.init();
	m_iterations = 0;
	m_minIterations = 2;
	m_maxIterations = 100;
	m_maxError = static_cast<Real>(0.01);
	m_iterationsV = 0;
	m_enableDivergenceSolver = true;
	m_maxIterationsV = 100;
	m_maxErrorV = static_cast<Real>(0.1);
	...
}
```

So upstream defaults are:
| Member | Default | Static param ID |
|---|---|---|
| `m_eps` | `1.0e-5` (compile-time const) | — |
| `m_iterations` | 0 | `SOLVER_ITERATIONS` (read-only) |
| `m_minIterations` | 2 | `MIN_ITERATIONS` (min 0) |
| `m_maxIterations` | 100 | `MAX_ITERATIONS` (min 1) |
| `m_maxError` | 0.01 (%) | `MAX_ERROR` (min 1e-6) |
| `m_iterationsV` | 0 | `SOLVER_ITERATIONS_V` (read-only) |
| `m_enableDivergenceSolver` | true | `USE_DIVERGENCE_SOLVER` |
| `m_maxIterationsV` | 100 | `MAX_ITERATIONS_V` (min 1) |
| `m_maxErrorV` | 0.1 (%) | `MAX_ERROR_V` (min 1e-6) |

### M.base-class machinery — `TimeStep.h`

Searched `TimeStep.h` and `TimeStep.cpp` for `m_minIterations`, `m_maxIterations`, `m_maxError`, `m_iterations` — none found. **All DFSPH convergence/iteration state lives in `TimeStepDFSPH` itself**, not the `TimeStep` base.

Full base header for reference:

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/TimeStep.h:1
#pragma once

#include "Common.h"
#include "ParameterObject.h"
#include "FluidModel.h"
#include "BoundaryModel.h"
#include "Discregrid/discrete_grid.hpp"

namespace SPH
{
	/** \brief Base class for the simulation methods. 
	*/
	class TimeStep : public GenParam::ParameterObject
	{
	protected:
		/** Clear accelerations and add gravitation.
		*/
		void clearAccelerations(const unsigned int fluidModelIndex);

		virtual void initParameters();

		void approximateNormal(Discregrid::DiscreteGrid* map, const Eigen::Vector3d &x, Eigen::Vector3d &n, const unsigned int dim);
		void computeVolumeAndBoundaryX(const unsigned int fluidModelIndex, const unsigned int i, const Vector3r &xi);
		void computeVolumeAndBoundaryX();
		void computeDensityAndGradient(const unsigned int fluidModelIndex, const unsigned int i, const Vector3r &xi);
		void computeDensityAndGradient();

	public:
		TimeStep();
		virtual ~TimeStep(void);

		/** Determine densities of all fluid particles.
		*/
		void computeDensities(const unsigned int fluidModelIndex);

		/** returns the name of the method */
		virtual std::string getMethodName() = 0;
		virtual void step() = 0;
		virtual void reset();

		virtual void init();
		virtual void resize() = 0;

		virtual void emittedParticles(FluidModel *model, const unsigned int startIndex) {};

		/** Important: First call m_model->performNeighborhoodSearchSort()
			 * to call the z_sort of the neighborhood search.
			 */
		virtual void performNeighborhoodSearchSort() {};

		virtual void saveState(BinaryFileWriter &binWriter) {};
		virtual void loadState(BinaryFileReader &binReader) {};

		virtual int getNumIterations() = 0;

#ifdef USE_PERFORMANCE_OPTIMIZATION
		void precomputeValues();
#endif
	};
}
```

`TimeStep` base contributes: `clearAccelerations(...)` (adds gravity to per-particle acceleration), `computeDensities(...)` (standard SPH density sum), `computeVolumeAndBoundaryX(...)` (Bender2019 volume-map setup), `computeDensityAndGradient(...)` (Koschier2017 density-map setup), and `precomputeValues()` (optional `V_j * ∇W_ij` cache). No iteration/error state.

---

## Section N — Gravity, viscosity, surface tension, position update

### N.1 — Gravity

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Added inside `clearAccelerations` (declared in `TimeStep.h:18`, called from `TimeStepDFSPH::step()` at `:177`):

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:173-178
	//////////////////////////////////////////////////////////////////////////
	// Reset accelerations and add gravity
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int fluidModelIndex = 0; fluidModelIndex < nModels; fluidModelIndex++)
		clearAccelerations(fluidModelIndex);
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
The actual gravity addition body lives in `TimeStep::clearAccelerations` (declared `TimeStep.h:18`, body in `TimeStep.cpp` — not quoted here per scope).

### N.2 — Viscosity / non-pressure forces

Delegated entirely through a plug-in mechanism:

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:179-182
	//////////////////////////////////////////////////////////////////////////
	// Compute all nonpressure forces like viscosity, vorticity, ...
	//////////////////////////////////////////////////////////////////////////
	sim->computeNonPressureForces();
```

The `Simulation::computeNonPressureForces()` implementation iterates over each fluid model's registered non-pressure-force modules (viscosity, vorticity, surface tension, drag, elasticity). The DFSPH solver does not hard-code any viscosity formula — selection is by the registered plug-in. Available DFSPH-compatible viscosity implementations include the subdirectory `references/SPlisHSPlasH/SPlisHSPlasH/Viscosity/` (e.g., `Viscosity_Standard.cpp`, `Viscosity_XSPH.cpp`, `Viscosity_Weiler2018.cpp`, `Viscosity_Bender2017.cpp` — not enumerated in detail per scope).

### N.3 — Surface tension / cohesion

Same delegation: rolled into `sim->computeNonPressureForces()` above. The DFSPH driver itself contains no surface-tension code. Surface-tension plug-ins live in `references/SPlisHSPlasH/SPlisHSPlasH/SurfaceTension/`.

### N.4 — Position update `x += dt·v`

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/DFSPH/TimeStepDFSPH.cpp:217-237
	//////////////////////////////////////////////////////////////////////////
	// compute final positions
	//////////////////////////////////////////////////////////////////////////
	for (unsigned int m = 0; m < nModels; m++)
	{
		FluidModel *fm = sim->getFluidModel(m);
		const unsigned int numParticles = fm->numActiveParticles();
		#pragma omp parallel default(shared)
		{
			#pragma omp for schedule(static)  
			for (int i = 0; i < (int)numParticles; i++)
			{
				if (fm->getParticleState(i) == ParticleState::Active)
				{
					Vector3r &xi = fm->getPosition(i);
					const Vector3r &vi = fm->getVelocity(i);
					xi += h * vi;
				}
			}
		}
	}
```

### N.summary — upstream ordering

```
performNeighborhoodSearch
[Bender2019 → computeVolumeAndBoundaryX; Koschier2017 → computeDensityAndGradient]
computeDensities × nModels
computeDFSPHFactor × nModels
if m_enableDivergenceSolver: divergenceSolve
clearAccelerations × nModels (resets a_i to gravity)
sim->computeNonPressureForces() (viscosity + vorticity + surface tension + drag + ...)
sim->updateTimeStepSize() (CFL update)
v += h * a (only non-pressure accels)
pressureSolve (modifies v in-place via pressure accel)
x += h * v
emitParticles / animateParticles
```

This means **gravity + viscosity + surface tension are applied to velocity BEFORE the pressure (density-constancy) solve**, but AFTER the divergence solve. The divergence solve operates on the pre-non-pressure-force velocity field.

---

## Section O — Boundary-model resolution

### O.1 — Default boundary method

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/Simulation.cpp:88
	m_boundaryHandlingMethod = static_cast<int>(BoundaryHandlingMethods::Bender2019);
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
(inside `Simulation::Simulation()` ctor at `Simulation.cpp:80-89`.) Setter declared at `Simulation.h:370`:
```cpp
void setBoundaryHandlingMethod(BoundaryHandlingMethods val) { m_boundaryHandlingMethod = (int) val; }
```

**Upstream default is `Bender2019` (volume maps), not Akinci2012 particles.**

### O.2 — Branch counts per scalar DFSPH function

Each scalar function takes exactly **one** `forall_*` boundary contribution per branch (mutually exclusive via `if / else if`):

| Function | Akinci2012 macro | Koschier2017 macro | Bender2019 macro |
|---|---|---|---|
| `computeDFSPHFactor` → adds to `grad_p_i` (and thus `sum_grad_p_k` via squared-norm at line 1172) | `forall_boundary_neighbors` (1×) | `forall_density_maps` (1×) | `forall_volume_maps` (1×) |
| `computeDensityAdv` → adds to `delta` | `forall_boundary_neighbors` (1×) | `forall_density_maps` (1×) | `forall_volume_maps` (1×) |
| `computeDensityChange` → adds to `densityAdv` | `forall_boundary_neighbors` (1×) | `forall_density_maps` (1×) | `forall_volume_maps` (1×) |
| `computePressureAccel` → adds to `ai` | `forall_boundary_neighbors` (1×) | `forall_density_maps` (1×) | `forall_volume_maps` (1×) |
| `compute_aij_pj` → adds to `aij_pj` | `forall_boundary_neighbors` (1×) | `forall_density_maps` (1×) | `forall_volume_maps` (1×) |

Total: 5 functions × 3 boundary methods = 15 boundary contribution sites. None of these is implemented in the GPU sph-water shader; the GPU shader uses only a hard AABB clamp (per probe-2 / Setup-1 G.6).

### O.3 — `BoundaryModel_Akinci2012` structure (subset for GPU-port relevance)

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Full header at `BoundaryModel_Akinci2012.h:1-116`:

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/BoundaryModel_Akinci2012.h:1
#ifndef __BoundaryModel_Akinci2012_h__
#define __BoundaryModel_Akinci2012_h__

#include "Common.h"
#include <vector>

#include "BoundaryModel.h"
#include "SPHKernels.h"


namespace SPH 
{	
	class TimeStep;

	/** \brief The boundary model stores the information required for boundary handling
	* using the approach of Akinci et al. 2012 [AIA+12].
	*
	* References:
	* - [AIA+12] Nadir Akinci, Markus Ihmsen, Gizem Akinci, Barbara Solenthaler, and Matthias Teschner. Versatile rigid-fluid coupling for incompressible SPH. ACM Trans. Graph., 31(4):62:1-62:8, July 2012. URL: http://doi.acm.org/10.1145/2185520.2185558
	*/
	class BoundaryModel_Akinci2012 : public BoundaryModel
	{
		public:
			BoundaryModel_Akinci2012();
			virtual ~BoundaryModel_Akinci2012();

		protected:
			bool m_sorted;
			unsigned int m_pointSetIndex;

			// values required for Akinci 2012 boundary handling
			std::vector<Vector3r> m_x0;
			std::vector<Vector3r> m_x;
			std::vector<Vector3r> m_v;
			std::vector<Real> m_V;

		public:
			unsigned int numberOfParticles() const { return static_cast<unsigned int>(m_x.size()); }
			unsigned int getPointSetIndex() const { return m_pointSetIndex; }
			bool isSorted() const { return m_sorted; }

			void computeBoundaryVolume();
			void resize(const unsigned int numBoundaryParticles);

			virtual void reset();

			virtual void performNeighborhoodSearchSort();

			virtual void saveState(BinaryFileWriter &binWriter);
			virtual void loadState(BinaryFileReader &binReader);

			void initModel(RigidBodyObject *rbo, const unsigned int numBoundaryParticles, Vector3r *boundaryParticles);
```

(getter/setter section truncated for brevity; full file is 116 lines, all getters/setters are simple `FORCE_INLINE` accessors over `m_x0`, `m_x`, `m_v`, `m_V`.)

Per-boundary-particle state:
- `m_x0[i]` — rest position (Vector3r).
- `m_x[i]` — current world position.
- `m_v[i]` — current velocity (relevant for dynamic / animated rigid bodies).
- `m_V[i]` — boundary particle "ψ-volume" (scalar Real).

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
Constructor body at `BoundaryModel_Akinci2012.cpp:13-21`:

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:13
BoundaryModel_Akinci2012::BoundaryModel_Akinci2012() :
	m_x0(),
	m_x(),
	m_v(),
	m_V()
{		
	m_sorted = false;
	m_pointSetIndex = 0;
}
```

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
`computeBoundaryVolume()` body at `BoundaryModel_Akinci2012.cpp:48-75`:

```cpp
// references/SPlisHSPlasH/SPlisHSPlasH/BoundaryModel_Akinci2012.cpp:48
void BoundaryModel_Akinci2012::computeBoundaryVolume()
{
	Simulation *sim = Simulation::getCurrent();
	const unsigned int nFluids = sim->numberOfFluidModels();
	NeighborhoodSearch *neighborhoodSearch = Simulation::getCurrent()->getNeighborhoodSearch();

	const unsigned int numBoundaryParticles = numberOfParticles();

	#pragma omp parallel default(shared)
	{
		#pragma omp for schedule(static)  
		for (int i = 0; i < (int)numBoundaryParticles; i++)
		{
			Real delta = sim->W_zero();
			for (unsigned int pid = nFluids; pid < sim->numberOfPointSets(); pid++)
			{
				BoundaryModel_Akinci2012 *bm_neighbor = static_cast<BoundaryModel_Akinci2012*>(sim->getBoundaryModelFromPointSet(pid));
				for (unsigned int j = 0; j < neighborhoodSearch->point_set(m_pointSetIndex).n_neighbors(pid, i); j++)
				{
					const unsigned int neighborIndex = neighborhoodSearch->point_set(m_pointSetIndex).neighbor(pid, i, j);
					delta += sim->W(getPosition(i) - bm_neighbor->getPosition(neighborIndex));
				}
			}
			const Real volume = static_cast<Real>(1.0) / delta;
			m_V[i] = volume;
		}
	}
}
```

Volume formula: `V_i(boundary) = 1 / Σ_{j ∈ boundary} W(x_i - x_j)`, including the self-term `W(0)`.

### O.summary — what a GPU Akinci port would need

A future GPU Akinci2012 port would require:
- Per-boundary-particle SSBOs for `x0`, `x`, `v`, `V` (3+3+3+1 = 10 floats / particle min; padded probably 16 floats).
- A neighborhood-search infrastructure that includes boundary point sets alongside fluid point sets (NHS already returns mixed-type neighbors via `numberOfNeighbors(fluidModelIndex, pid, ...)` where `pid >= nFluids` is boundary).
- A one-time `computeBoundaryVolume` dispatch over boundary particles (similar to fluid density, but uses `W` not `∇W`).
- The 5 sites in §O.2 each need a second neighbor loop pass over boundary particles. On GPU this typically means either a second cellgrid containing boundary particles or a unified cellgrid with type tags.

---

## Section P — Incidental findings

<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **Kernel `m_l` normalization unit**: per `SPHKernels.h:32` upstream defines `m_l = 48 / (π * h³)` — *not* `48 / (π * h⁴)`. The radial `1/h` to make `gradW` have units of inverse-length-per-position arrives via `gradq /= m_radius` at `SPHKernels.h:70`. If our host-side code (`density_alpha.comp.glsl:5` and probe-1 Section F) bakes `48/(π h⁴)` into the GPU UBO directly, the numerical match depends on whether the shader also divides by `h` separately or whether the host folded it in pre-emptively. **Flag for architect-1 to verify.**
- **Pressure solver uses `m_minIterations = 2` default; divergence solver hard-codes `< 1` (i.e., minimum 1 iteration)**. These are not symmetric: a GPU port that uses `MAX_PRESSURE_ITERATIONS` and `MAX_DIVERGENCE_ITERATIONS` with a single shared `min_iter` knob would not faithfully mirror upstream.
- **Two different `eta` formulas**:
  - Pressure: `eta = m_maxError * 0.01 * density0` (dimensionless density delta).
  - Divergence: `eta = (1/h) * m_maxErrorV * 0.01 * density0` (density-rate, hence the `1/h`).
  These differ by the factor `1/h`. Important for our GPU convergence-check (commit 3): the divergence check is comparing a rate, not a density delta.
- **`pressureSolveIteration` filters by `ParticleState::Active`** (line 578) but **`divergenceSolveIteration` does not** (no such check after line 651). Worth flagging if the GPU port has identical kernels for both.
- **`aij_pj` is scaled differently by caller**: pressure caller multiplies by `h²` (`:582`); divergence caller multiplies by `h` (`:656`). So the same `compute_aij_pj` is reused but the iteration math differs by one power of h. Easy to miss when porting.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **Warmstart behavior**: `USE_WARMSTART` and `USE_WARMSTART_V` are `#define`-on by default at `TimeStepDFSPH.h:9-10`. The warmstart keeps the previous frame's `p/ρ²` (scaled by `h²`) and re-injects it at the start of the next frame. If our GPU port omits warmstart, expect higher per-frame iteration counts (the upstream initial iterates already encode pressure history; cold-start always begins at zero).
- **Particle-deficiency clamp at `numNeighbors < 20` (3D) / `< 7` (2D)**: applied in both the divergence outer-loop init (`:421-435`) and inside `divergenceSolveIteration` (`:676-690`). NOT applied in the density-constancy solver. So density-constancy "ignores" particle deficiency; divergence-solve actively zeroes affected particles' contribution.
- **`std::vector<std::vector<Real>>& pressure_rho2` parameter to `computePressureAccel`**: the function accesses `pressure_rho2[fluidModelIndex][i]` AND `pressure_rho2[pid][neighborIndex]` (inside `forall_fluid_neighbors`). So this is a per-fluid-model, per-particle pressure array, indexed by both the local model and the neighbor's model. GPU equivalent likely needs a SSBO indexed similarly (single-phase port can flatten).
- **`fm_neighbor->getDensity0() / density0` in `computePressureAccel`** (line 1322): used to symmetrize cross-phase pressure contributions. For a single-fluid port this term is always 1.0 and can be removed.
- **The `forall_density_maps` and `forall_volume_maps` loops are per-rigid-body, not per-particle**: their inner content executes **once per boundary body**, not once per neighbor particle. This affects how a GPU port would dispatch — a single uniform contribution per (fluid particle, boundary body) pair rather than a true neighbor iteration.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **`Simulation::Bender2019` is the default boundary method** (`Simulation.cpp:88`); this is what most upstream tutorials assume. Akinci2012 is the older legacy/research method but the most common reference in introductory SPH literature.
- **No `m_simulationData` declaration visible in `TimeStepDFSPH.h` beyond `SimulationDataDFSPH m_simulationData;` at line 27**: per-particle scratch state (factor, densityAdv, pressureRho2, pressureRho2_V, pressureAccel) lives in `SimulationDataDFSPH.{h,cpp}` (not opened in this probe per scope). Probe-1 likely covered this — architect-1 should cross-reference.
<!-- integrity-allow: cat1.intra-repo; audit-doc snapshot of pre-v1 codebase (see grandfather-catalog audit-citation); n/a -->
<!-- integrity-allow: cat1.bare-path; audit-doc snapshot bare-path citation pre-v1.2 (see grandfather-catalog audit-bare-path); n/a -->
- **AVX vs scalar divergence**: not verified in this probe (scalar-only per architect-1 instruction). The AVX twins at `TimeStepDFSPH.cpp:735-1103` are nominally numerically identical but use packed 8-wide gather/scatter; if a future probe needs to confirm bit-exactness, it is worth comparing reduction order (AVX may produce slightly different rounding due to associativity).

---

**End of probe-3 report.**
