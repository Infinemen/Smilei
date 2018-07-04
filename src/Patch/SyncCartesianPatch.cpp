
#include "SyncCartesianPatch.h"

#include <vector>

#include "Domain.h"
#include "VectorPatch.h"
#include "Params.h"
#include "SmileiMPI.h"
#include "PatchesFactory.h"

using namespace std;

void SyncCartesianPatch::patchedToCartesian( VectorPatch& vecPatches, Domain& domain, Params &params, SmileiMPI* smpi, Timers &timers, int itime )
{
    timers.grids.restart();

    // Loop / local_patches_ -> OK
    //     put()

    // Loop / additional_patches_ (identify where goes the additionnal patches regarding patch_count)
    //     patch().send()
    for ( unsigned int i=0 ; i<domain.additional_patches_.size() ; i++ ) {
        //cout << smpi->getRank() << " will send " << domain.additional_patches_[i] << " to " << domain.additional_patches_ranks[i] << endl;
        unsigned int ipatch = domain.additional_patches_[i]-vecPatches.refHindex_;

        SyncCartesianPatch::sendPatchedToCartesian( vecPatches(ipatch)->EMfields, domain.additional_patches_[i], domain.additional_patches_ranks[i], smpi, vecPatches(ipatch) );

        // who = smpi->hrank_THEORIQUE( domain.additional_patches_[i] );
        //     MANAGED WITH --> domain.additional_patches_ranks !!!
        // Send( vecPatches(ipatch)->EMfields->Jx_, who )
    }


    // Loop / missing_patches_ (identify where are the missing patches)
    //     domain.recv( domain.missing_patches_[i] from smpi->hrank( domain.missing_patches_[i] ) )
    //     put to domain
    for ( unsigned int i=0 ; i<domain.missing_patches_.size() ; i++ ) {
        unsigned int ipatch = domain.missing_patches_[i]-vecPatches.refHindex_;
        //cout << smpi->getRank() << " will recv " << domain.missing_patches_[i] << " from " << domain.missing_patches_ranks[i] << endl;

        SyncCartesianPatch::recvPatchedToCartesian( domain.patch_->EMfields, domain.missing_patches_[i], domain.missing_patches_ranks[i], vecPatches, params, smpi, domain );

    }


    for ( unsigned int i=0 ; i<domain.additional_patches_.size() ; i++ ) {
        //cout << smpi->getRank() << " will send " << domain.additional_patches_[i] << endl;
        //cout << smpi->getRank() << " finaliser send " << domain.additional_patches_[i] << " to " << domain.additional_patches_ranks[i] << endl;
        unsigned int ipatch = domain.additional_patches_[i]-vecPatches.refHindex_;

        SyncCartesianPatch::finalize_sendPatchedToCartesian( vecPatches(ipatch)->EMfields, domain.additional_patches_[i], domain.additional_patches_ranks[i], smpi, vecPatches(ipatch) );

        // who = smpi->hrank_THEORIQUE( domain.additional_patches_[i] );
        //     MANAGED WITH --> domain.additional_patches_ranks !!!
        // Send( vecPatches(ipatch)->EMfields->Jx_, who )
    }

    //cout << smpi->getRank() << " - "; 
    //for ( unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++ ) {
    for ( unsigned int i=0 ; i<domain.local_patches_.size() ; i++ ) {
        unsigned int ipatch = domain.local_patches_[i]-vecPatches.refHindex_;
        //cout << ipatch << " ";
        //vecPatches(ipatch)->EMfields->Ex_->put( domain.patch_->EMfields->Ex_, params, smpi, vecPatches(ipatch), domain.patch_ );
        //vecPatches(ipatch)->EMfields->Ey_->put( domain.patch_->EMfields->Ey_, params, smpi, vecPatches(ipatch), domain.patch_ );
        //vecPatches(ipatch)->EMfields->Ez_->put( domain.patch_->EMfields->Ez_, params, smpi, vecPatches(ipatch), domain.patch_ );
	//
        //vecPatches(ipatch)->EMfields->Bx_->put( domain.patch_->EMfields->Bx_, params, smpi, vecPatches(ipatch), domain.patch_ );
        //vecPatches(ipatch)->EMfields->By_->put( domain.patch_->EMfields->By_, params, smpi, vecPatches(ipatch), domain.patch_ );
        //vecPatches(ipatch)->EMfields->Bz_->put( domain.patch_->EMfields->Bz_, params, smpi, vecPatches(ipatch), domain.patch_ );

        vecPatches(ipatch)->EMfields->Jx_->put( domain.patch_->EMfields->Jx_, params, smpi, vecPatches(ipatch), domain.patch_ );
        vecPatches(ipatch)->EMfields->Jy_->put( domain.patch_->EMfields->Jy_, params, smpi, vecPatches(ipatch), domain.patch_ );
        vecPatches(ipatch)->EMfields->Jz_->put( domain.patch_->EMfields->Jz_, params, smpi, vecPatches(ipatch), domain.patch_ );
	if(params.is_spectral){
          vecPatches(ipatch)->EMfields->rho_->put( domain.patch_->EMfields->rho_, params, smpi, vecPatches(ipatch), domain.patch_ );
          // useless rho_old is save directly on vecPatches concerned by the Maxwell soler see VectorPatches::solveMaxwell()
          //vecPatches(ipatch)->EMfields->rhoold_->put( domain.patch_->EMfields->rhoold_, params, smpi, vecPatches(ipatch),
	  //domain.patch_ );
	}

    }
    //cout << endl;
    timers.grids.update();
}

void SyncCartesianPatch::sendPatchedToCartesian( ElectroMagn* localfields, unsigned int hindex, int send_to_global_patch_rank, SmileiMPI* smpi, Patch* patch )
{
    //smpi->send( localfields->Jx_, hindex, send_to_global_patch_rank );
    //    isend( EM->Bz_m, to, mpi_tag+tag, requests[tag]); tag++;
    smpi->isend( localfields->Jx_, send_to_global_patch_rank, hindex*3  , patch->requests_[0] );
    smpi->isend( localfields->Jy_, send_to_global_patch_rank, hindex*3+1, patch->requests_[1] );
    smpi->isend( localfields->Jz_, send_to_global_patch_rank, hindex*3+2, patch->requests_[2] );
    

    //if(params.is_spectral){
    //    smpi->send( localfields->rho_, hindex, global_patch_rank, params, smpi );
    //
    //}
}

void SyncCartesianPatch::finalize_sendPatchedToCartesian( ElectroMagn* localfields, unsigned int hindex, int send_to_global_patch_rank, SmileiMPI* smpi, Patch* patch )
{
    MPI_Status status;
    MPI_Wait( &(patch->requests_[0]), &status );
    MPI_Wait( &(patch->requests_[1]), &status );
    MPI_Wait( &(patch->requests_[2]), &status );
}

void SyncCartesianPatch::recvPatchedToCartesian( ElectroMagn* globalfields, unsigned int hindex, int local_patch_rank, VectorPatch& vecPatches, Params &params, SmileiMPI* smpi, Domain& domain )
{
    // Jx_
    // define fake_patch
    unsigned int n_moved = 0;
    //Patch* fake_patch = PatchesFactory::clone(vecPatches(0), params, smpi, vecPatches.domain_decomposition_, hindex, n_moved, false);
    //smpi->recv( fake_patch->Jx_, hindex, local_patch_rank );
    //    recv(  EM->Bz_m, from, tag ); tag++;

    //smpi->recv( fake_patch->EMfields->Jx_, local_patch_rank, hindex );

    //  vecPatches(ipatch) -> need sender patch coordinates : vecPatches.getDomainCoordinates( hindex )
    // Buffer will be resized for each component, fake local patch, which wil have Jxyz, and coordinates to update ?
    domain.fake_patch->hindex = hindex;
    domain.fake_patch->Pcoordinates = vecPatches.domain_decomposition_->getDomainCoordinates( hindex );

    smpi->recv( domain.fake_patch->EMfields->Jx_, local_patch_rank, hindex*3 );
    domain.fake_patch->EMfields->Jx_->put( globalfields->Jx_, params, smpi, domain.fake_patch, domain.patch_ );

    smpi->recv( domain.fake_patch->EMfields->Jy_, local_patch_rank, hindex*3+1 );
    domain.fake_patch->EMfields->Jy_->put( globalfields->Jy_, params, smpi, domain.fake_patch, domain.patch_ );

    smpi->recv( domain.fake_patch->EMfields->Jz_, local_patch_rank, hindex*3+2 );
    domain.fake_patch->EMfields->Jz_->put( globalfields->Jz_, params, smpi, domain.fake_patch, domain.patch_ );


    //if(params.is_spectral){
    //    smpi->send( localfields->rho_, hindex, global_patch_rank, params, smpi );
    //
    //}

    //delete fake_patch;

}


void SyncCartesianPatch::cartesianToPatches( Domain& domain, VectorPatch& vecPatches, Params &params, SmileiMPI* smpi, Timers &timers, int itime )
{
    timers.grids.restart();

    // Loop / additional_patches_ (regarding cartesian), get data from cartesian
    for ( unsigned int i=0 ; i<domain.additional_patches_.size() ; i++ ) {
        unsigned int ipatch = domain.additional_patches_[i]-vecPatches.refHindex_;

        SyncCartesianPatch::recvCartesianToPatches( vecPatches(ipatch)->EMfields, domain.additional_patches_[i], domain.additional_patches_ranks[i], smpi,  vecPatches(ipatch) );

    }


    // Loop / missing_patches_ (regarding cartesian), send data which do not concern myself
    //     patch().send()
    for ( unsigned int i=0 ; i<domain.missing_patches_.size() ; i++ ) {
        unsigned int ipatch = domain.missing_patches_[i]-vecPatches.refHindex_;

        SyncCartesianPatch::sendCartesianToPatches( domain.patch_->EMfields, domain.missing_patches_[i], domain.missing_patches_ranks[i], vecPatches, params, smpi, domain );

    }


    for ( unsigned int i=0 ; i<domain.additional_patches_.size() ; i++ ) {
        unsigned int ipatch = domain.additional_patches_[i]-vecPatches.refHindex_;

        SyncCartesianPatch::finalize_recvCartesianToPatches( vecPatches(ipatch)->EMfields, domain.additional_patches_[i], domain.additional_patches_ranks[i], smpi, vecPatches(ipatch) );

    }



    //for ( unsigned int ipatch=0 ; ipatch<vecPatches.size() ; ipatch++ ) {
    for ( unsigned int i=0 ; i<domain.local_patches_.size() ; i++ ) {
        unsigned int ipatch = domain.local_patches_[i]-vecPatches.refHindex_;

        vecPatches(ipatch)->EMfields->Ex_->get( domain.patch_->EMfields->Ex_, params, smpi, domain.patch_, vecPatches(ipatch) );
        vecPatches(ipatch)->EMfields->Ey_->get( domain.patch_->EMfields->Ey_, params, smpi, domain.patch_, vecPatches(ipatch) );
        vecPatches(ipatch)->EMfields->Ez_->get( domain.patch_->EMfields->Ez_, params, smpi, domain.patch_, vecPatches(ipatch) );
   
        vecPatches(ipatch)->EMfields->Bx_m->get( domain.patch_->EMfields->Bx_m, params, smpi, domain.patch_, vecPatches(ipatch) );
        vecPatches(ipatch)->EMfields->By_m->get( domain.patch_->EMfields->By_m, params, smpi, domain.patch_, vecPatches(ipatch) );
        vecPatches(ipatch)->EMfields->Bz_m->get( domain.patch_->EMfields->Bz_m, params, smpi, domain.patch_, vecPatches(ipatch) );

//        vecPatches(ipatch)->EMfields->Bx_->get( domain.patch_->EMfields->Bx_, params, smpi, domain.patch_, vecPatches(ipatch) );
//        vecPatches(ipatch)->EMfields->By_->get( domain.patch_->EMfields->By_, params, smpi, domain.patch_, vecPatches(ipatch) );
//        vecPatches(ipatch)->EMfields->Bz_->get( domain.patch_->EMfields->Bz_, params, smpi, domain.patch_, vecPatches(ipatch) );

//        vecPatches(ipatch)->EMfields->Jx_->get( domain.patch_->EMfields->Jx_, params, smpi, domain.patch_, vecPatches(ipatch) );
//        vecPatches(ipatch)->EMfields->Jy_->get( domain.patch_->EMfields->Jy_, params, smpi, domain.patch_, vecPatches(ipatch) );
//        vecPatches(ipatch)->EMfields->Jz_->get( domain.patch_->EMfields->Jz_, params, smpi, domain.patch_, vecPatches(ipatch) );
//	if(params.is_spectral) {
//          vecPatches(ipatch)->EMfields->rho_->get( domain.patch_->EMfields->rho_, params, smpi, domain.patch_, vecPatches(ipatch) );
//          vecPatches(ipatch)->EMfields->rhoold_->get( domain.patch_->EMfields->rhoold_, params, smpi, domain.patch_,
//	  vecPatches(ipatch) );
//	}

    }

    timers.grids.update();
}


void SyncCartesianPatch::recvCartesianToPatches( ElectroMagn* localfields, unsigned int hindex, int recv_from_global_patch_rank, SmileiMPI* smpi, Patch* patch )
{
    smpi->irecv( localfields->Ex_, recv_from_global_patch_rank, hindex*6  , patch->requests_[0] );
    smpi->irecv( localfields->Ey_, recv_from_global_patch_rank, hindex*6+1, patch->requests_[1] );
    smpi->irecv( localfields->Ez_, recv_from_global_patch_rank, hindex*6+2, patch->requests_[2] );
   

    smpi->irecv( localfields->Bx_m, recv_from_global_patch_rank, hindex*6+3, patch->requests_[3] );
    smpi->irecv( localfields->By_m, recv_from_global_patch_rank, hindex*6+4, patch->requests_[4] );
    smpi->irecv( localfields->Bz_m, recv_from_global_patch_rank, hindex*6+5, patch->requests_[5] );

}

void SyncCartesianPatch::finalize_recvCartesianToPatches( ElectroMagn* localfields, unsigned int hindex, int recv_from_global_patch_rank, SmileiMPI* smpi, Patch* patch )
{
    MPI_Status status;
    MPI_Wait( &(patch->requests_[0]), &status );
    MPI_Wait( &(patch->requests_[1]), &status );
    MPI_Wait( &(patch->requests_[2]), &status );
    MPI_Wait( &(patch->requests_[3]), &status );
    MPI_Wait( &(patch->requests_[4]), &status );
    MPI_Wait( &(patch->requests_[5]), &status );
}

void SyncCartesianPatch::sendCartesianToPatches( ElectroMagn* globalfields, unsigned int hindex, int local_patch_rank, VectorPatch& vecPatches, Params &params, SmileiMPI* smpi, Domain& domain )
{
    // Jx_
    // define fake_patch
    unsigned int n_moved = 0;
    //Patch* fake_patch = PatchesFactory::clone(vecPatches(0), params, smpi, vecPatches.domain_decomposition_, hindex, n_moved, false);
    //smpi->recv( fake_patch->Jx_, hindex, local_patch_rank );
    //    recv(  EM->Bz_m, from, tag ); tag++;

    //smpi->recv( fake_patch->EMfields->Jx_, local_patch_rank, hindex );

    //  vecPatches(ipatch) -> need sender patch coordinates : vecPatches.getDomainCoordinates( hindex )
    // Buffer will be resized for each component, fake local patch, which wil have Jxyz, and coordinates to update ?


    domain.fake_patch->hindex = hindex;
    domain.fake_patch->Pcoordinates = vecPatches.domain_decomposition_->getDomainCoordinates( hindex );

    domain.fake_patch->EMfields->Ex_->get( globalfields->Ex_, params, smpi, domain.patch_, domain.fake_patch );
    smpi->send( domain.fake_patch->EMfields->Ex_, local_patch_rank, hindex*6 );

    domain.fake_patch->EMfields->Ey_->get( globalfields->Ey_, params, smpi, domain.patch_, domain.fake_patch );
    smpi->send( domain.fake_patch->EMfields->Ey_, local_patch_rank, hindex*6+1 );

    domain.fake_patch->EMfields->Ez_->get( globalfields->Ez_, params, smpi, domain.patch_, domain.fake_patch );
    smpi->send( domain.fake_patch->EMfields->Ez_, local_patch_rank, hindex*6+2 );

    domain.fake_patch->EMfields->Bx_m->get( globalfields->Bx_m, params, smpi, domain.patch_, domain.fake_patch );
    smpi->send( domain.fake_patch->EMfields->Bx_m, local_patch_rank, hindex*6+3 );

    domain.fake_patch->EMfields->By_m->get( globalfields->By_m, params, smpi, domain.patch_, domain.fake_patch );
    smpi->send( domain.fake_patch->EMfields->By_m, local_patch_rank, hindex*6+4 );

    domain.fake_patch->EMfields->Bz_m->get( globalfields->Bz_m, params, smpi, domain.patch_, domain.fake_patch );
    smpi->send( domain.fake_patch->EMfields->Bz_m, local_patch_rank, hindex*6+5 );


    //if(params.is_spectral){
    //    smpi->send( localfields->rho_, hindex, global_patch_rank, params, smpi );
    //
    //}

    //delete fake_patch;
}


