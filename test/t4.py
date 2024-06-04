import hddm_r

for rec in hddm_r.istream("root://nod25.phys.uconn.edu/Gluex/simulation" +
                          "/simsamples/particle_gun-v5.2.0/particle_gun001_019_rest.hddm"):
   for pe in rec.getReconstructedPhysicsEvents():
      print(f"found run {pe.runNo}, event {pe.eventNo}")
