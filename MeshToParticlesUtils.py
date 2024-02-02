import maya.cmds as mc


def meshToParticles(meshes=None):
    if meshes is None:
        meshes = mc.ls(sl=True, dag=True, shapes=True, type="mesh")
    if not meshes:
        mc.warning("No mesh selected.")
        return
    
    for mesh in meshes:
        setupParticles(mesh + '.outMesh')



def setupParticles(mesh_attr, create_attr=True, name="particle"):
    """Creates particles from a given mesh attribute"""

    # Create meshToParticles
    mtp = mc.createNode('meshToParticles', ss=True)
    mc.setAttr(mtp + '.ca', create_attr)
    mc.connectAttr(mesh_attr, mtp + '.inMesh')
    
    # Create particle shape
    particle = mc.createNode('particle', n=name + 'Shape', ss=True)
    mc.setAttr(particle + '.isDynamic', 0)
    mc.setAttr(particle + '.startFrame', 10000000)
    mc.setAttr(particle + '.particleRenderType', 4)

    # Connect
    mc.connectAttr(mtp + ".output", particle + '.currentTime')

    return mtp, particle


def bifrostToParticles(bif_attr):
    """Creates particles from a Bifrost graph attribute"""
    print("creating particles from", bif_attr)
    bif = mc.createNode('bifrostGeoToMaya', ss=True)
    mc.setAttr(bif + '.ct', "", type="string")
    mc.setAttr(bif + '.pr', "* !point_position", type="string")
    mc.connectAttr(bif_attr, bif + '.bifrostGeo')
    setupParticles(bif + '.mayaMesh[0]', name=bif_attr.split('.')[1])


def bifrostToParticlesDialog():
    """Dialog to selecting a Bifrost graph port to create particles from"""
    containers = mc.ls(sl=True, dag=True, shapes=True, type='bifrostGraphShape')
    if not containers:
        containers = mc.ls(sl=True, type='bifrostBoard')
        if not containers:
            mc.warning("No bifrost container selected.")
            return
    container = containers[0]
    ports = mc.vnnCompound(container, "/", lp=True, op=True)

    # Create dialog
    dialog = mc.window(title="Bifrost To Particles", widthHeight=(200, 100), retain=False)
    mc.columnLayout(adjustableColumn=True)

    def createAndClose(p):
        bifrostToParticles(p)
        mc.deleteUI(dialog)

    # Create button for each port
    for port in ports:
        mc.button(label=port, command=lambda x=None, p=f"{container}.{port}": createAndClose(p))

    mc.showWindow(dialog)