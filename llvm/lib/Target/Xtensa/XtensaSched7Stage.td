def Xtensa7StageModel : SchedMachineModel {
  let MicroOpBufferSize = 0; // Explicitly set to zero since Xtensa is in-order.
  let IssueWidth = 1;        // 1 micro-op is dispatched per cycle.
  let LoadLatency = 2;       // This is overriden by OperandCycles if the
                             // Itineraries are queried instead.
  let MispredictPenalty = 3; // According to the Xtensa LX Handbook, taken
                             // branches to aligned targets incur a 3-cycle
                             // penalty on the 7-stage pipeline. There may be
                             // an additional cycle when the target is
                             // unaligned, but let's assume that hot blocks will
                             // be aligned anyway.
  let CompleteModel = true;
}

def XtensaUnitALU    : ProcResource<1> { let BufferSize = 0; }
def XtensaUnitMul    : ProcResource<1> { let BufferSize = 0; }
def XtensaUnitDiv    : ProcResource<1> { let BufferSize = 0; }
def XtensaUnitBranch : ProcResource<1> { let BufferSize = 0; }
def XtensaUnitLd     : ProcResource<1> { let BufferSize = 0; }

let SchedModel = Xtensa7StageModel in {

def : WriteRes<WriteALU, [XtensaUnitALU]>;
def : WriteRes<WriteSAR, [XtensaUnitALU]>;
def : ReadAdvance<ReadSAR, 0>;

def : WriteRes<WriteMul, [XtensaUnitMul]> {
  let Latency = 2;
  // Multiplier isn't pipelined based on gcc's cost model
  let ResourceCycles = [2];
}

def : WriteRes<WriteDiv, [XtensaUnitDiv]> {
  let Latency = 16;
  // Divider isn't pipelined based on gcc's cost model
  let ResourceCycles = [16];
}

def : WriteRes<WriteBranch, [XtensaUnitBranch]> {
  // According to the Xtensa LX Handbook, taken branches on the 7-stage pipeline
  // have a base penalty of 3 cycles. If the target is not aligned to the
  // processor's fetch width, an extra cycle is incurred.
  // Untaken branches do not have any penalties, so let's settle for 2 as a sane
  // default.
  let Latency = 2;
}

def : WriteRes<WriteL32R, [XtensaUnitLd]>;
def : WriteRes<WriteLd, [XtensaUnitLd]> {
  let Latency = 2;
}

def : InstRW<[WriteALU], (instrs COPY)>;

}
