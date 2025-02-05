from gluex import hddm_r

for rec in hddm_r.istream("http://nod25.phys.uconn.edu:2880/Gluex/simulation" +
                          "/simsamples/particle_gun-v5.2.0/particle_gun001_019_rest.hddm"):
   for pe in rec.getReconstructedPhysicsEvents():
      print(f"found run {pe.runNo}, event {pe.eventNo}")

for rec in hddm_r.istream("https://nod25.phys.uconn.edu:2843/Gluex/simulation" +
                          "/simsamples/particle_gun-v5.2.0/particle_gun001_019_rest.hddm"):
   for pe in rec.getReconstructedPhysicsEvents():
      print(f"then found run {pe.runNo}, event {pe.eventNo}")
